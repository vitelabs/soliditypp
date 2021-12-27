// SPDX-License-Identifier: GPL-3.0

#include <libsolidity/analysis/DeclarationTypeChecker.h>

#include <libsolidity/analysis/ConstantEvaluator.h>

#include <libsolidity/ast/TypeProvider.h>

#include <liblangutil/ErrorReporter.h>

#include <libsolutil/Algorithms.h>

#include <boost/range/adaptor/transformed.hpp>

using namespace std;
using namespace solidity::langutil;
using namespace solidity::frontend;

bool DeclarationTypeChecker::visit(ElementaryTypeName const& _typeName)
{
	if (_typeName.annotation().type)
		return false;

	_typeName.annotation().type = TypeProvider::fromElementaryTypeName(_typeName.typeName());
	if (_typeName.stateMutability().has_value())
	{
		// for non-address types this was already caught by the parser
		solAssert(_typeName.annotation().type->category() == Type::Category::Address, "");
		switch (*_typeName.stateMutability())
		{
			case StateMutability::Payable:
				_typeName.annotation().type = TypeProvider::payableAddress();
				break;
			case StateMutability::NonPayable:
				_typeName.annotation().type = TypeProvider::address();
				break;
			default:
				m_errorReporter.typeError(
					2311_error,
					_typeName.location(),
					"Address types can only be payable or non-payable."
				);
				break;
		}
	}
	return true;
}

bool DeclarationTypeChecker::visit(EnumDefinition const& _enum)
{
	if (_enum.members().size() > 256)
		m_errorReporter.declarationError(
			1611_error,
			_enum.location(),
			"Enum with more than 256 members is not allowed."
		);

	return false;
}

bool DeclarationTypeChecker::visit(StructDefinition const& _struct)
{
	if (_struct.annotation().recursive.has_value())
	{
		if (!m_currentStructsSeen.empty() && *_struct.annotation().recursive)
			m_recursiveStructSeen = true;
		return false;
	}

	if (m_currentStructsSeen.count(&_struct))
	{
		_struct.annotation().recursive = true;
		m_recursiveStructSeen = true;
		return false;
	}

	bool previousRecursiveStructSeen = m_recursiveStructSeen;
	bool hasRecursiveChild = false;

	m_currentStructsSeen.insert(&_struct);

	for (auto const& member: _struct.members())
	{
		m_recursiveStructSeen = false;
		member->accept(*this);
		solAssert(member->annotation().type, "");
		solAssert(member->annotation().type->canBeStored(), "Type cannot be used in struct.");
		if (m_recursiveStructSeen)
			hasRecursiveChild = true;
	}

	if (!_struct.annotation().recursive.has_value())
		_struct.annotation().recursive = hasRecursiveChild;
	m_recursiveStructSeen = previousRecursiveStructSeen || *_struct.annotation().recursive;
	m_currentStructsSeen.erase(&_struct);
	if (m_currentStructsSeen.empty())
		m_recursiveStructSeen = false;

	// Check direct recursion, fatal error if detected.
	auto visitor = [&](StructDefinition const& _struct, auto& _cycleDetector, size_t _depth)
	{
		if (_depth >= 256)
			m_errorReporter.fatalDeclarationError(
				5651_error,
				_struct.location(),
				"Struct definition exhausts cyclic dependency validator."
			);

		for (ASTPointer<VariableDeclaration> const& member: _struct.members())
		{
			Type const* memberType = member->annotation().type;

			if (auto arrayType = dynamic_cast<ArrayType const*>(memberType))
				memberType = arrayType->finalBaseType(true);

			if (auto structType = dynamic_cast<StructType const*>(memberType))
				if (_cycleDetector.run(structType->structDefinition()))
					return;
		}
	};
	if (util::CycleDetector<StructDefinition>(visitor).run(_struct))
		m_errorReporter.fatalTypeError(2046_error, _struct.location(), "Recursive struct definition.");

	return false;
}

void DeclarationTypeChecker::endVisit(UserDefinedTypeName const& _typeName)
{
	if (_typeName.annotation().type)
		return;

	Declaration const* declaration = _typeName.pathNode().annotation().referencedDeclaration;
	solAssert(declaration, "");

	if (StructDefinition const* structDef = dynamic_cast<StructDefinition const*>(declaration))
	{
		if (!m_insideFunctionType && !m_currentStructsSeen.empty())
			structDef->accept(*this);
		_typeName.annotation().type = TypeProvider::structType(*structDef, DataLocation::Storage);
	}
	else if (EnumDefinition const* enumDef = dynamic_cast<EnumDefinition const*>(declaration))
		_typeName.annotation().type = TypeProvider::enumType(*enumDef);
	else if (ContractDefinition const* contract = dynamic_cast<ContractDefinition const*>(declaration))
		_typeName.annotation().type = TypeProvider::contract(*contract);
	else
	{
		_typeName.annotation().type = TypeProvider::emptyTuple();
		m_errorReporter.fatalTypeError(
			5172_error,
			_typeName.location(),
			"Name has to refer to a struct, enum or contract."
		);
	}
}

void DeclarationTypeChecker::endVisit(IdentifierPath const& _path)
{
	Declaration const* declaration = _path.annotation().referencedDeclaration;
	solAssert(declaration, "");

	if (ContractDefinition const* contract = dynamic_cast<ContractDefinition const*>(declaration))
		if (contract->isLibrary())
			m_errorReporter.typeError(1130_error, _path.location(), "Invalid use of a library name.");
}

bool DeclarationTypeChecker::visit(FunctionTypeName const& _typeName)
{
	if (_typeName.annotation().type)
		return false;

	bool previousInsideFunctionType = m_insideFunctionType;
	m_insideFunctionType = true;
	_typeName.parameterTypeList()->accept(*this);
	_typeName.returnParameterTypeList()->accept(*this);
	m_insideFunctionType = previousInsideFunctionType;

	switch (_typeName.visibility())
	{
		case Visibility::Internal:
		case Visibility::External:
			break;
		default:
			m_errorReporter.fatalTypeError(
				6012_error,
				_typeName.location(),
				"Invalid visibility, can only be \"external\" or \"internal\"."
			);
			return false;
	}

	if (_typeName.isPayable() && _typeName.visibility() != Visibility::External)
	{
		m_errorReporter.fatalTypeError(
			7415_error,
			_typeName.location(),
			"Only external function types can be payable."
		);
		return false;
	}
	_typeName.annotation().type = TypeProvider::function(_typeName);
	return false;
}

void DeclarationTypeChecker::endVisit(Mapping const& _mapping)
{
	if (_mapping.annotation().type)
		return;

	if (auto const* typeName = dynamic_cast<UserDefinedTypeName const*>(&_mapping.keyType()))
		switch (typeName->annotation().type->category())
		{
			case Type::Category::Enum:
			case Type::Category::Contract:
				break;
			default:
				m_errorReporter.fatalTypeError(
					7804_error,
					typeName->location(),
					"Only elementary types, contract types or enums are allowed as mapping keys."
				);
				break;
		}
	else
		solAssert(dynamic_cast<ElementaryTypeName const*>(&_mapping.keyType()), "");

	TypePointer keyType = _mapping.keyType().annotation().type;
	TypePointer valueType = _mapping.valueType().annotation().type;

	// Convert key type to memory.
	keyType = TypeProvider::withLocationIfReference(DataLocation::Memory, keyType);

	// Convert value type to storage reference.
	valueType = TypeProvider::withLocationIfReference(DataLocation::Storage, valueType);
	_mapping.annotation().type = TypeProvider::mapping(keyType, valueType);
}

void DeclarationTypeChecker::endVisit(ArrayTypeName const& _typeName)
{
	if (_typeName.annotation().type)
		return;

	TypePointer baseType = _typeName.baseType().annotation().type;
	if (!baseType)
	{
		solAssert(!m_errorReporter.errors().empty(), "");
		return;
	}

	solAssert(baseType->storageBytes() != 0, "Illegal base type of storage size zero for array.");
	if (Expression const* length = _typeName.length())
	{
		optional<rational> lengthValue;
		if (length->annotation().type && length->annotation().type->category() == Type::Category::RationalNumber)
			lengthValue = dynamic_cast<RationalNumberType const&>(*length->annotation().type).value();
		else if (optional<ConstantEvaluator::TypedRational> value = ConstantEvaluator::evaluate(m_errorReporter, *length))
			lengthValue = value->value;

		if (!lengthValue || lengthValue > TypeProvider::uint256()->max())
			m_errorReporter.typeError(
				5462_error,
				length->location(),
				"Invalid array length, expected integer literal or constant expression."
			);
		else if (*lengthValue == 0)
			m_errorReporter.typeError(1406_error, length->location(), "Array with zero length specified.");
		else if (lengthValue->denominator() != 1)
			m_errorReporter.typeError(3208_error, length->location(), "Array with fractional length specified.");
		else if (*lengthValue < 0)
			m_errorReporter.typeError(3658_error, length->location(), "Array with negative length specified.");

		_typeName.annotation().type = TypeProvider::array(
			DataLocation::Storage,
			baseType,
			lengthValue ? u256(lengthValue->numerator()) : u256(0)
		);
	}
	else
		_typeName.annotation().type = TypeProvider::array(DataLocation::Storage, baseType);
}

void DeclarationTypeChecker::endVisit(VariableDeclaration const& _variable)
{
	if (_variable.annotation().type)
		return;

	if (_variable.isFileLevelVariable() && !_variable.isConstant())
		m_errorReporter.declarationError(
			8342_error,
			_variable.location(),
			"Only constant variables are allowed at file level."
		);
	if (_variable.isConstant() && (!_variable.isStateVariable() && !_variable.isFileLevelVariable()))
		m_errorReporter.declarationError(
			1788_error,
			_variable.location(),
			"The \"constant\" keyword can only be used for state variables or variables at file level."
		);
	if (_variable.immutable() && !_variable.isStateVariable())
		m_errorReporter.declarationError(
			8297_error,
			_variable.location(),
			"The \"immutable\" keyword can only be used for state variables."
		);

	using Location = VariableDeclaration::Location;
	Location varLoc = _variable.referenceLocation();
	DataLocation typeLoc = DataLocation::Memory;

	set<Location> allowedDataLocations = _variable.allowedDataLocations();
	if (!allowedDataLocations.count(varLoc))
	{
		auto locationToString = [](VariableDeclaration::Location _location) -> string
		{
			switch (_location)
			{
				case Location::Memory: return "\"memory\"";
				case Location::Storage: return "\"storage\"";
				case Location::CallData: return "\"calldata\"";
				case Location::Unspecified: return "none";
			}
			return {};
		};

		string errorString;
		if (!_variable.hasReferenceOrMappingType())
			errorString = "Data location can only be specified for array, struct or mapping types";
		else
		{
			errorString = "Data location must be " +
				util::joinHumanReadable(
					allowedDataLocations | boost::adaptors::transformed(locationToString),
					", ",
					" or "
				);
			if (_variable.isConstructorParameter())
				errorString += " for constructor parameter";
			else if (_variable.isCallableOrCatchParameter())
				errorString +=
					" for " +
					string(_variable.isReturnParameter() ? "return " : "") +
					"parameter in" +
					string(_variable.isExternalCallableParameter() ? " external" : "") +
					" function";
			else
				errorString += " for variable";
		}
		errorString += ", but " + locationToString(varLoc) + " was given.";
		m_errorReporter.typeError(6651_error, _variable.location(), errorString);

		solAssert(!allowedDataLocations.empty(), "");
		varLoc = *allowedDataLocations.begin();
	}

	// Find correct data location.
	if (_variable.isEventParameter())
	{
		solAssert(varLoc == Location::Unspecified, "");
		typeLoc = DataLocation::Memory;
	}

	else if (_variable.isFileLevelVariable())
	{
		solAssert(varLoc == Location::Unspecified, "");
		typeLoc = DataLocation::Memory;
	}
	else if (_variable.isStateVariable())
	{
		solAssert(varLoc == Location::Unspecified, "");
		typeLoc = (_variable.isConstant() || _variable.immutable()) ? DataLocation::Memory : DataLocation::Storage;
	}
	else if (
		dynamic_cast<StructDefinition const*>(_variable.scope()) ||
		dynamic_cast<EnumDefinition const*>(_variable.scope())
	)
		// The actual location will later be changed depending on how the type is used.
		typeLoc = DataLocation::Storage;
	else
		switch (varLoc)
		{
			case Location::Memory:
				typeLoc = DataLocation::Memory;
				break;
			case Location::Storage:
				typeLoc = DataLocation::Storage;
				break;
			case Location::CallData:
				typeLoc = DataLocation::CallData;
				break;
			case Location::Unspecified:
				solAssert(!_variable.hasReferenceOrMappingType(), "Data location not properly set.");
		}

	TypePointer type = _variable.typeName().annotation().type;
	if (auto ref = dynamic_cast<ReferenceType const*>(type))
	{
		bool isPointer = !_variable.isStateVariable();
		type = TypeProvider::withLocation(ref, typeLoc, isPointer);
	}

	if (_variable.isConstant() && !type->isValueType())
	{
		bool allowed = false;
		if (auto arrayType = dynamic_cast<ArrayType const*>(type))
			allowed = arrayType->isByteArray();
		if (!allowed)
			m_errorReporter.fatalDeclarationError(9259_error, _variable.location(), "Constants of non-value type not yet implemented.");
	}

	_variable.annotation().type = type;
}

bool DeclarationTypeChecker::visit(UsingForDirective const& _usingFor)
{
	ContractDefinition const* library = dynamic_cast<ContractDefinition const*>(
		_usingFor.libraryName().annotation().referencedDeclaration
	);

	if (!library || !library->isLibrary())
		m_errorReporter.fatalTypeError(4357_error, _usingFor.libraryName().location(), "Library name expected.");

	if (_usingFor.typeName())
		_usingFor.typeName()->accept(*this);

	return false;
}

bool DeclarationTypeChecker::visit(InheritanceSpecifier const& _inheritanceSpecifier)
{
	auto const* contract = dynamic_cast<ContractDefinition const*>(_inheritanceSpecifier.name().annotation().referencedDeclaration);
	solAssert(contract, "");
	if (contract->isLibrary())
	{
		m_errorReporter.typeError(
			2571_error,
			_inheritanceSpecifier.name().location(),
			"Libraries cannot be inherited from."
		);
		return false;
	}
	return true;
}

bool DeclarationTypeChecker::check(ASTNode const& _node)
{
	auto watcher = m_errorReporter.errorWatcher();
	_node.accept(*this);
	return watcher.ok();
}
