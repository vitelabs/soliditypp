// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ Type analyzer and checker.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#include <libsolidity/analysis/SolidityppTypeChecker.h>
#include <libsolidity/ast/SolidityppAST.h>
#include <libsolidity/ast/TypeProvider.h>

#include <libyul/AsmAnalysis.h>
#include <libyul/AsmAnalysisInfo.h>
#include <libyul/AST.h>

#include <liblangutil/ErrorReporter.h>

#include <libsolutil/Algorithms.h>
#include <libsolutil/StringUtils.h>
#include <range/v3/view/drop_exactly.hpp>

#include <memory>
#include <vector>

using namespace std;
using namespace solidity;
using namespace solidity::util;
using namespace solidity::langutil;
using namespace solidity::frontend;

bool SolidityppTypeChecker::visit(SourceUnit const& _sourceUnit)
{
    m_sourceLanguage = *_sourceUnit.annotation().sourceLanguage;
    return true;
}

bool SolidityppTypeChecker::visit(FunctionDefinition const& _function)
{
	if (_function.markedVirtual())
	{
		if (_function.isFree())
			m_errorReporter.syntaxError(4493_error, _function.location(), "Free functions cannot be virtual.");
		else if (_function.isConstructor())
			m_errorReporter.typeError(7001_error, _function.location(), "Constructors cannot be virtual.");
		else if (_function.annotation().contract->isInterface())
			m_errorReporter.warning(5815_error, _function.location(), "Interface functions are implicitly \"virtual\"");
		else if (_function.visibility() == Visibility::Private)
			m_errorReporter.typeError(3942_error, _function.location(), "\"virtual\" and \"private\" cannot be used together.");
		else if (_function.libraryFunction())
			m_errorReporter.typeError(7801_error, _function.location(), "Library functions cannot be \"virtual\".");
	}
	if (_function.overrides() && _function.isFree())
		m_errorReporter.syntaxError(1750_error, _function.location(), "Free functions cannot override.");

	if (!_function.modifiers().empty() && _function.isFree())
		m_errorReporter.syntaxError(5811_error, _function.location(), "Free functions cannot have modifiers.");

	if (_function.isPayable())
	{
		if (_function.libraryFunction())
			m_errorReporter.typeError(7708_error, _function.location(), "Library functions cannot be payable.");
		else if (_function.isFree())
			m_errorReporter.typeError(9559_error, _function.location(), "Free functions cannot be payable.");
		else if (_function.isOrdinary() && !_function.isPartOfExternalInterface())
			m_errorReporter.typeError(5587_error, _function.location(), "\"internal\" and \"private\" functions cannot be payable.");
	}

	// Solidity++: check async functions
//	if (_function.isAsync())
//    {
//        if (_function.libraryFunction())
//            m_errorReporter.typeError(100301_error, _function.location(), "Library functions cannot be async.");
//        else if (_function.isFree())
//            m_errorReporter.typeError(100302_error, _function.location(), "Free functions cannot be async.");
//        else if (_function.isOrdinary() && !_function.isPartOfExternalInterface())
//            m_errorReporter.typeError(100303_error, _function.location(), "\"internal\" and \"private\" functions cannot be async.");
//    }
	// Solidity++: check externally visible functions (must be async)
//	else if (_function.isPartOfExternalInterface())
//    {
//	    if (_function.isOrdinary() && !_function.libraryFunction())
//            m_errorReporter.typeError(100304_error, _function.location(), "\"external\" and \"public\" functions must be async.");
//    }

	vector<VariableDeclaration const*> internalParametersInConstructor;

	auto checkArgumentAndReturnParameter = [&](VariableDeclaration const& _var) {
		if (type(_var)->containsNestedMapping())
			if (_var.referenceLocation() == VariableDeclaration::Location::Storage)
				solAssert(
					_function.libraryFunction() || _function.isConstructor() || !_function.isPublic(),
					"Mapping types for parameters or return variables "
					"can only be used in internal or library functions."
				);
		bool functionIsExternallyVisible =
			(!_function.isConstructor() && _function.isPublic()) ||
			(_function.isConstructor() && !m_currentContract->abstract());
		if (
			_function.isConstructor() &&
			_var.referenceLocation() == VariableDeclaration::Location::Storage &&
			!m_currentContract->abstract()
		)
			m_errorReporter.typeError(
				3644_error,
				_var.location(),
				"This parameter has a type that can only be used internally. "
				"You can make the contract abstract to avoid this problem."
			);
		else if (functionIsExternallyVisible)
		{
			auto iType = type(_var)->interfaceType(_function.libraryFunction());

			if (!iType)
			{
				string message = iType.message();
				solAssert(!message.empty(), "Expected detailed error message!");
				if (_function.isConstructor())
					message += " You can make the contract abstract to avoid this problem.";
				m_errorReporter.typeError(4103_error, _var.location(), message);
			}
			else if (
				!useABICoderV2() &&
				!typeSupportedByOldABIEncoder(*type(_var), _function.libraryFunction())
			)
			{
				string message =
					"This type is only supported in ABI coder v2. "
					"Use \"pragma abicoder v2;\" to enable the feature.";
				if (_function.isConstructor())
					message +=
						" Alternatively, make the contract abstract and supply the "
						"constructor arguments from a derived contract.";
				m_errorReporter.typeError(
					4957_error,
					_var.location(),
					message
				);
			}
		}
	};
	for (ASTPointer<VariableDeclaration> const& var: _function.parameters())
	{
		checkArgumentAndReturnParameter(*var);
		var->accept(*this);
	}
	for (ASTPointer<VariableDeclaration> const& var: _function.returnParameters())
	{
		checkArgumentAndReturnParameter(*var);
		var->accept(*this);
	}

	set<Declaration const*> modifiers;
	for (ASTPointer<ModifierInvocation> const& modifier: _function.modifiers())
	{
		vector<ContractDefinition const*> baseContracts;
		if (auto contract = dynamic_cast<ContractDefinition const*>(_function.scope()))
		{
			baseContracts = contract->annotation().linearizedBaseContracts;
			// Delete first base which is just the main contract itself
			baseContracts.erase(baseContracts.begin());
		}

		visitManually(
			*modifier,
			_function.isConstructor() ? baseContracts : vector<ContractDefinition const*>()
		);
		Declaration const* decl = &dereference(modifier->name());
		if (modifiers.count(decl))
		{
			if (dynamic_cast<ContractDefinition const*>(decl))
				m_errorReporter.declarationError(1697_error, modifier->location(), "Base constructor already provided.");
		}
		else
			modifiers.insert(decl);
	}

	solAssert(_function.isFree() == !m_currentContract, "");
	if (!m_currentContract)
	{
		solAssert(!_function.isConstructor(), "");
		solAssert(!_function.isFallback(), "");
		solAssert(!_function.isReceive(), "");
	}
	else if (m_currentContract->isInterface())
	{
		if (_function.isImplemented())
			m_errorReporter.typeError(4726_error, _function.location(), "Functions in interfaces cannot have an implementation.");

		if (_function.isConstructor())
			m_errorReporter.typeError(6482_error, _function.location(), "Constructor cannot be defined in interfaces.");
		else if (_function.visibility() != Visibility::External)
			m_errorReporter.typeError(1560_error, _function.location(), "Functions in interfaces must be declared external.");
	}
	else if (m_currentContract->contractKind() == ContractKind::Library)
		if (_function.isConstructor())
			m_errorReporter.typeError(7634_error, _function.location(), "Constructor cannot be defined in libraries.");

	if (_function.isImplemented())
		_function.body().accept(*this);
	else if (_function.isConstructor())
		m_errorReporter.typeError(5700_error, _function.location(), "Constructor must be implemented if declared.");
	else if (_function.libraryFunction())
		m_errorReporter.typeError(9231_error, _function.location(), "Library functions must be implemented if declared.");
	else if (!_function.virtualSemantics())
	{
		if (_function.isFree())
			solAssert(m_errorReporter.hasErrors(), "");
		else
			m_errorReporter.typeError(5424_error, _function.location(), "Functions without implementation must be marked virtual.");
	}


	if (_function.isFallback())
		typeCheckFallbackFunction(_function);
	else if (_function.isReceive())
		typeCheckReceiveFunction(_function);
	else if (_function.isConstructor())
		typeCheckConstructor(_function);

	return false;
}

bool SolidityppTypeChecker::visit(FunctionCall const& _functionCall)
{
	vector<ASTPointer<Expression const>> const& arguments = _functionCall.arguments();
	bool argumentsArePure = true;

	// We need to check arguments' type first as they will be needed for overload resolution.
	for (ASTPointer<Expression const> const& argument: arguments)
	{
		argument->accept(*this);
		if (!*argument->annotation().isPure)
			argumentsArePure = false;
	}

	// Store argument types - and names if given - for overload resolution
	{
		FuncCallArguments funcCallArgs;

		funcCallArgs.names = _functionCall.names();

		for (ASTPointer<Expression const> const& argument: arguments)
			funcCallArgs.types.push_back(type(*argument));

		_functionCall.expression().annotation().arguments = std::move(funcCallArgs);
	}

	_functionCall.expression().accept(*this);

	Type const* expressionType = type(_functionCall.expression());

	// Determine function call kind and function type for this FunctionCall node
	FunctionCallAnnotation& funcCallAnno = _functionCall.annotation();
	FunctionTypePointer functionType = nullptr;
	funcCallAnno.isConstant = false;

	bool isLValue = false;

	// Determine and assign function call kind, lvalue, purity and function type for this FunctionCall node
	switch (expressionType->category())
	{
	case Type::Category::Function:
		functionType = dynamic_cast<FunctionType const*>(expressionType);
		funcCallAnno.kind = FunctionCallKind::FunctionCall;

		// Solidity++: determine function call synchrony (sync / async)
		if (m_sourceLanguage == SourceLanguage::Solidity)
		    funcCallAnno.async = false;  // All function calls in Solidity are synchronous

		if (auto memberAccess = dynamic_cast<MemberAccess const*>(&_functionCall.expression()))
		{
			if (dynamic_cast<FunctionDefinition const*>(memberAccess->annotation().referencedDeclaration))
				_functionCall.expression().annotation().calledDirectly = true;
		}
		else if (auto identifier = dynamic_cast<Identifier const*>(&_functionCall.expression()))
			if (dynamic_cast<FunctionDefinition const*>(identifier->annotation().referencedDeclaration))
				_functionCall.expression().annotation().calledDirectly = true;

		// Purity for function calls also depends upon the callee and its FunctionType
		funcCallAnno.isPure =
			argumentsArePure &&
			*_functionCall.expression().annotation().isPure &&
			functionType->isPure();

		if (
			functionType->kind() == FunctionType::Kind::ArrayPush ||
			functionType->kind() == FunctionType::Kind::ByteArrayPush
		)
			isLValue = functionType->parameterTypes().empty();

		break;

	case Type::Category::TypeType:
	{
		// Determine type for type conversion or struct construction expressions
		TypePointer const& actualType = dynamic_cast<TypeType const&>(*expressionType).actualType();
		solAssert(!!actualType, "");

		if (actualType->category() == Type::Category::Struct)
		{
			if (actualType->containsNestedMapping())
				m_errorReporter.fatalTypeError(
					9515_error,
					_functionCall.location(),
					"Struct containing a (nested) mapping cannot be constructed."
				);
			functionType = dynamic_cast<StructType const&>(*actualType).constructorType();
			funcCallAnno.kind = FunctionCallKind::StructConstructorCall;
		}
		else
		{
			if (auto const* contractType = dynamic_cast<ContractType const*>(actualType))
				if (contractType->isSuper())
					m_errorReporter.fatalTypeError(
						1744_error,
						_functionCall.location(),
						"Cannot convert to the super type."
					);
			funcCallAnno.kind = FunctionCallKind::TypeConversion;
		}

		funcCallAnno.isPure = argumentsArePure;

		break;
	}

	default:
		m_errorReporter.fatalTypeError(5704_error, _functionCall.location(), "Type is not callable");
		// Unreachable, because fatalTypeError throws. We don't set kind, but that's okay because the switch below
		// is never reached. And, even if it was, SetOnce would trigger an assertion violation and not UB.
		funcCallAnno.isPure = argumentsArePure;
		break;
	}

	funcCallAnno.isLValue = isLValue;

	// Determine return types
	switch (*funcCallAnno.kind)
	{
	case FunctionCallKind::TypeConversion:
		funcCallAnno.type = typeCheckTypeConversionAndRetrieveReturnType(_functionCall);
		break;

	case FunctionCallKind::StructConstructorCall: // fall-through
	case FunctionCallKind::FunctionCall:
	{
		TypePointers returnTypes;

		switch (functionType->kind())
		{
		case FunctionType::Kind::ABIDecode:
		{
			returnTypes = typeCheckABIDecodeAndRetrieveReturnType(
				_functionCall,
				useABICoderV2()
			);
			break;
		}
		case FunctionType::Kind::ABIEncode:
		case FunctionType::Kind::ABIEncodePacked:
		case FunctionType::Kind::ABIEncodeWithSelector:
		case FunctionType::Kind::ABIEncodeWithSignature:
		{
			typeCheckABIEncodeFunctions(_functionCall, functionType);
			returnTypes = functionType->returnParameterTypes();
			break;
		}
		case FunctionType::Kind::MetaType:
			returnTypes = typeCheckMetaTypeFunctionAndRetrieveReturnType(_functionCall);
			break;
		default:
		{
			typeCheckFunctionCall(_functionCall, functionType);
			returnTypes = m_evmVersion.supportsReturndata() ?
				functionType->returnParameterTypes() :
				functionType->returnParameterTypesWithoutDynamicTypes();
			break;
		}
		}

		funcCallAnno.type = returnTypes.size() == 1 ?
			move(returnTypes.front()) :
			TypeProvider::tuple(move(returnTypes));

		break;
	}

	default:
		// for non-callables, ensure error reported and annotate node to void function
		solAssert(m_errorReporter.hasErrors(), "");
		funcCallAnno.kind = FunctionCallKind::FunctionCall;
		funcCallAnno.type = TypeProvider::emptyTuple();
		break;
	}

	return false;
}

bool SolidityppTypeChecker::visit(FunctionCallOptions const& _functionCallOptions)
{
	solAssert(_functionCallOptions.options().size() == _functionCallOptions.names().size(), "Lengths of name & value arrays differ!");

	_functionCallOptions.expression().annotation().arguments = _functionCallOptions.annotation().arguments;

	_functionCallOptions.expression().accept(*this);

	_functionCallOptions.annotation().isPure = false;
	_functionCallOptions.annotation().isConstant = false;
	_functionCallOptions.annotation().isLValue = false;

	auto expressionFunctionType = dynamic_cast<FunctionType const*>(type(_functionCallOptions.expression()));
	if (!expressionFunctionType)
	{
		m_errorReporter.fatalTypeError(2622_error, _functionCallOptions.location(), "Expected callable expression before call options.");
		return false;
	}

	bool setSalt = false;
	bool setValue = false;
	bool setGas = false;
	bool setTokenId = false;

	FunctionType::Kind kind = expressionFunctionType->kind();
	if (
		kind != FunctionType::Kind::Creation &&
		kind != FunctionType::Kind::External &&
		kind != FunctionType::Kind::BareCall &&
		kind != FunctionType::Kind::BareCallCode &&
		kind != FunctionType::Kind::BareDelegateCall &&
		kind != FunctionType::Kind::BareStaticCall
	)
	{
		m_errorReporter.fatalTypeError(
			2193_error,
			_functionCallOptions.location(),
			"Function call options can only be set on external function calls or contract creations."
		);
		return false;
	}

	if (
		expressionFunctionType->valueSet() ||
		expressionFunctionType->tokenSet() ||
		expressionFunctionType->gasSet() ||
		expressionFunctionType->saltSet()
	)
		m_errorReporter.typeError(
			1645_error,
			_functionCallOptions.location(),
			"Function call options have already been set, you have to combine them into a single "
			"{...}-option."
		);

	auto setCheckOption = [&](bool& _option, string const& _name)
	{
		if (_option)
			m_errorReporter.typeError(
				9886_error,
				_functionCallOptions.location(),
				"Duplicate option \"" + std::move(_name) + "\"."
			);

		_option = true;
	};

	for (size_t i = 0; i < _functionCallOptions.names().size(); ++i)
	{
		string const& name = *(_functionCallOptions.names()[i]);

        // Solidity++: disable ethereum options of "salt" and "gas"
		if (name == "gas" || name == "salt")
		{
            m_errorReporter.typeError(
                    100201_error,
                    _functionCallOptions.location(),
                    "Cannot set option \"" + name + "\" in Solidity++. Valid options are \"token\" and \"value\"."
            );
		}
		else if (name == "value")
		{
			if (kind == FunctionType::Kind::BareDelegateCall)
				m_errorReporter.typeError(
					6189_error,
					_functionCallOptions.location(),
					"Cannot set option \"value\" for delegatecall."
				);
			else if (kind == FunctionType::Kind::BareStaticCall)
				m_errorReporter.typeError(
					2842_error,
					_functionCallOptions.location(),
					"Cannot set option \"value\" for staticcall."
				);
			else if (!expressionFunctionType->isPayable())
				m_errorReporter.typeError(
					7006_error,
					_functionCallOptions.location(),
					kind == FunctionType::Kind::Creation ?
						"Cannot set option \"value\", since the constructor of " +
						expressionFunctionType->returnParameterTypes().front()->toString() +
						" is not payable." :
						"Cannot set option \"value\" on a non-payable function type."
				);
			else
			{
				expectType(*_functionCallOptions.options()[i], *TypeProvider::uint256());

		        setCheckOption(setValue, "value");
		    }
		}
		// Solidity++:
        else if (name == "token")
        {
            if (kind == FunctionType::Kind::BareDelegateCall)
                m_errorReporter.typeError(
                        6189_error,
                        _functionCallOptions.location(),
                        "Cannot set option \"token\" for delegatecall."
                );
            else if (kind == FunctionType::Kind::BareStaticCall)
                m_errorReporter.typeError(
                        2842_error,
                        _functionCallOptions.location(),
                        "Cannot set option \"token\" for staticcall."
                );
            else if (!expressionFunctionType->isPayable())
                m_errorReporter.typeError(
                        7006_error,
                        _functionCallOptions.location(),
                        kind == FunctionType::Kind::Creation ?
                        "Cannot set option \"token\", since the constructor of " +
                        expressionFunctionType->returnParameterTypes().front()->toString() +
                        " is not payable." :
                        "Cannot set option \"token\" on a non-payable function type."
                );
            else
            {
                expectType(*_functionCallOptions.options()[i], *TypeProvider::viteTokenId());

                setCheckOption(setTokenId, "token");
            }
        }

		else
			m_errorReporter.typeError(
				9318_error,
				_functionCallOptions.location(),
				"Unknown call option \"" + name + "\". Valid options are \"token\" and \"value\"."
			);
	}

	if (setSalt && !m_evmVersion.hasCreate2())
		m_errorReporter.typeError(
			5189_error,
			_functionCallOptions.location(),
			"Unsupported call option \"salt\" (requires Constantinople-compatible VMs)."
		);

	_functionCallOptions.annotation().type = expressionFunctionType->copyAndSetCallOptions(setGas, setValue, setSalt, setTokenId);
	return false;
}

// Solidity++:
bool SolidityppTypeChecker::visit(AwaitExpression const& _awaitExpression)
{
    auto call = dynamic_cast<FunctionCall const*>(&_awaitExpression.expression());
    solAssert(call, "fail to convert function call in await expression: " + _awaitExpression.location().text());

    call->accept(*this);
    _awaitExpression.annotation().type = _awaitExpression.expression().annotation().type;
    _awaitExpression.annotation().isPure = false;

    call->annotation().async = false;  // set to sync

    return false;
}

void SolidityppTypeChecker::endVisit(Literal const& _literal)
{
	if (_literal.looksLikeViteAddress()) // Solidity++: check vite address literal
	{
		_literal.annotation().type = TypeProvider::address();

		string msg;

		if (!_literal.passesViteAddressChecksum())
		{
			msg = "This looks like a Vite address but has an invalid checksum.";
		}

		if (!msg.empty())
			m_errorReporter.syntaxError(
				9429_error,
				_literal.location(),
				msg
			);
	}

	if (_literal.looksLikeViteTokenId()) // Solidity++: check token id literal
	{
		_literal.annotation().type = TypeProvider::viteTokenId();

		string msg;

		if (!_literal.passesViteTokenIdChecksum())
		{
			msg = "This looks like a Vite Token Id but has an invalid checksum.";
		}

		if (!msg.empty())
			m_errorReporter.syntaxError(
				9429_error,
				_literal.location(),
				msg
			);
	}

	if (_literal.isHexNumber() && _literal.subDenomination() != Literal::SubDenomination::None)
		m_errorReporter.fatalTypeError(
			5145_error,
			_literal.location(),
			"Hexadecimal numbers cannot be used with unit denominations. "
			"You can use an expression of the form \"0x1234 * 1 day\" instead."
		);

	if (_literal.subDenomination() == Literal::SubDenomination::Year)
		m_errorReporter.typeError(
			4820_error,
			_literal.location(),
			"Using \"years\" as a unit denomination is deprecated."
		);

	if (!_literal.annotation().type)
		_literal.annotation().type = TypeProvider::forLiteral(_literal);

	if (!_literal.annotation().type)
		m_errorReporter.fatalTypeError(2826_error, _literal.location(), "Invalid literal value.");

	_literal.annotation().isPure = true;
	_literal.annotation().isLValue = false;
	_literal.annotation().isConstant = false;
}
