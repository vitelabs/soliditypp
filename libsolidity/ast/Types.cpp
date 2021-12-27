// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ data types
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#include <libsolidity/ast/Types.h>

#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/TypeProvider.h>

#include <libsolidity/analysis/ConstantEvaluator.h>

#include <libsolutil/Algorithms.h>
#include <libsolutil/CommonData.h>
#include <libsolutil/CommonIO.h>
#include <libsolutil/FunctionSelector.h>
#include <libsolutil/Blake2.h>
#include <libsolutil/UTF8.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

#include <range/v3/view/enumerate.hpp>

#include <limits>
#include <unordered_set>
#include <utility>

using namespace std;
using namespace solidity;
using namespace solidity::langutil;
using namespace solidity::frontend;

namespace
{

/// Checks whether _mantissa * (10 ** _expBase10) fits into 4096 bits.
bool fitsPrecisionBase10(bigint const& _mantissa, uint32_t _expBase10)
{
	double const log2Of10AwayFromZero = 3.3219280948873624;
	return fitsPrecisionBaseX(_mantissa, log2Of10AwayFromZero, _expBase10);
}

/// Checks whether _value fits into IntegerType _type.
BoolResult fitsIntegerType(bigint const& _value, IntegerType const& _type)
{
	if (_value < 0 && !_type.isSigned())
		return BoolResult::err("Cannot implicitly convert signed literal to unsigned type.");

	if (_type.minValue() > _value || _value > _type.maxValue())
		return BoolResult::err("Literal is too large to fit in " + _type.toString(false) + ".");

	return true;
}

/// Checks whether _value fits into _bits bits when having 1 bit as the sign bit
/// if _signed is true.
bool fitsIntoBits(bigint const& _value, unsigned _bits, bool _signed)
{
	return fitsIntegerType(
		_value,
		*TypeProvider::integer(
			_bits,
			_signed ? IntegerType::Modifier::Signed : IntegerType::Modifier::Unsigned
		)
	);
}

util::Result<TypePointers> transformParametersToExternal(TypePointers const& _parameters, bool _inLibrary)
{
	TypePointers transformed;

	for (auto const& type: _parameters)
	{
		if (!type)
			return util::Result<TypePointers>::err("Type information not present.");
		else if (TypePointer ext = type->interfaceType(_inLibrary).get())
			transformed.push_back(ext);
		else
			return util::Result<TypePointers>::err("Parameter should have external type.");
	}

	return transformed;
}

}

void Type::clearCache() const
{
	m_members.clear();
	m_stackItems.reset();
	m_stackSize.reset();
}

void StorageOffsets::computeOffsets(TypePointers const& _types)
{
	bigint slotOffset = 0;
	unsigned byteOffset = 0;
	map<size_t, pair<u256, unsigned>> offsets;
	for (size_t i = 0; i < _types.size(); ++i)
	{
		Type const* type = _types[i];
		if (!type->canBeStored())
			continue;
		if (byteOffset + type->storageBytes() > 32)
		{
			// would overflow, go to next slot
			++slotOffset;
			byteOffset = 0;
		}
		solAssert(slotOffset < bigint(1) << 256 ,"Object too large for storage.");
		offsets[i] = make_pair(u256(slotOffset), byteOffset);
		solAssert(type->storageSize() >= 1, "Invalid storage size.");
		if (type->storageSize() == 1 && byteOffset + type->storageBytes() <= 32)
			byteOffset += type->storageBytes();
		else
		{
			slotOffset += type->storageSize();
			byteOffset = 0;
		}
	}
	if (byteOffset > 0)
		++slotOffset;
	solAssert(slotOffset < bigint(1) << 256, "Object too large for storage.");
	m_storageSize = u256(slotOffset);
	swap(m_offsets, offsets);
}

pair<u256, unsigned> const* StorageOffsets::offset(size_t _index) const
{
	if (m_offsets.count(_index))
		return &m_offsets.at(_index);
	else
		return nullptr;
}

void MemberList::combine(MemberList const & _other)
{
	m_memberTypes += _other.m_memberTypes;
}

pair<u256, unsigned> const* MemberList::memberStorageOffset(string const& _name) const
{
	StorageOffsets const& offsets = storageOffsets();

	for (auto&& [index, member]: m_memberTypes | ranges::views::enumerate)
		if (member.name == _name)
			return offsets.offset(index);
	return nullptr;
}

u256 const& MemberList::storageSize() const
{
	return storageOffsets().storageSize();
}

StorageOffsets const& MemberList::storageOffsets() const {
	return m_storageOffsets.init([&]{
		TypePointers memberTypes;
		memberTypes.reserve(m_memberTypes.size());
		for (auto const& member: m_memberTypes)
			memberTypes.push_back(member.type);

		StorageOffsets storageOffsets;
		storageOffsets.computeOffsets(memberTypes);

		return storageOffsets;
	});
}

/// Helper functions for type identifier
namespace
{

string parenthesizeIdentifier(string const& _internal)
{
	return "(" + _internal + ")";
}

template <class Range>
string identifierList(Range const&& _list)
{
	return parenthesizeIdentifier(boost::algorithm::join(_list, ","));
}

string richIdentifier(Type const* _type)
{
	return _type ? _type->richIdentifier() : "";
}

string identifierList(vector<TypePointer> const& _list)
{
	return identifierList(_list | boost::adaptors::transformed(richIdentifier));
}

string identifierList(Type const* _type)
{
	return parenthesizeIdentifier(richIdentifier(_type));
}

string identifierList(Type const* _type1, Type const* _type2)
{
	TypePointers list;
	list.push_back(_type1);
	list.push_back(_type2);
	return identifierList(list);
}

string parenthesizeUserIdentifier(string const& _internal)
{
	return parenthesizeIdentifier(_internal);
}

}

string Type::escapeIdentifier(string const& _identifier)
{
	string ret = _identifier;
	// FIXME: should be _$$$_
	boost::algorithm::replace_all(ret, "$", "$$$");
	boost::algorithm::replace_all(ret, ",", "_$_");
	boost::algorithm::replace_all(ret, "(", "$_");
	boost::algorithm::replace_all(ret, ")", "_$");
	return ret;
}

string Type::identifier() const
{
	string ret = escapeIdentifier(richIdentifier());
	solAssert(ret.find_first_of("0123456789") != 0, "Identifier cannot start with a number.");
	solAssert(
		ret.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMONPQRSTUVWXYZ_$") == string::npos,
		"Identifier contains invalid characters."
	);
	return ret;
}

TypePointer Type::commonType(Type const* _a, Type const* _b)
{
	if (!_a || !_b)
		return nullptr;
	else if (_a->mobileType() && _b->isImplicitlyConvertibleTo(*_a->mobileType()))
		return _a->mobileType();
	else if (_b->mobileType() && _a->isImplicitlyConvertibleTo(*_b->mobileType()))
		return _b->mobileType();
	else
		return nullptr;
}

MemberList const& Type::members(ASTNode const* _currentScope) const
{
	if (!m_members[_currentScope])
	{
		solAssert(
			_currentScope == nullptr ||
			dynamic_cast<SourceUnit const*>(_currentScope) ||
			dynamic_cast<ContractDefinition const*>(_currentScope),
		"");
		MemberList::MemberMap members = nativeMembers(_currentScope);
		if (_currentScope)
			members += boundFunctions(*this, *_currentScope);
		m_members[_currentScope] = make_unique<MemberList>(move(members));
	}
	return *m_members[_currentScope];
}

TypePointer Type::fullEncodingType(bool _inLibraryCall, bool _encoderV2, bool) const
{
	TypePointer encodingType = mobileType();
	if (encodingType)
		encodingType = encodingType->interfaceType(_inLibraryCall);
	if (encodingType)
		encodingType = encodingType->encodingType();
	// Structs are fine in the following circumstances:
	// - ABIv2 or,
	// - storage struct for a library
	if (_inLibraryCall && encodingType && encodingType->dataStoredIn(DataLocation::Storage))
		return encodingType;
	TypePointer baseType = encodingType;
	while (auto const* arrayType = dynamic_cast<ArrayType const*>(baseType))
	{
		baseType = arrayType->baseType();

		auto const* baseArrayType = dynamic_cast<ArrayType const*>(baseType);
		if (!_encoderV2 && baseArrayType && baseArrayType->isDynamicallySized())
			return nullptr;
	}
	if (!_encoderV2 && dynamic_cast<StructType const*>(baseType))
		return nullptr;

	return encodingType;
}

MemberList::MemberMap Type::boundFunctions(Type const& _type, ASTNode const& _scope)
{
	vector<UsingForDirective const*> usingForDirectives;
	if (auto const* sourceUnit = dynamic_cast<SourceUnit const*>(&_scope))
		usingForDirectives += ASTNode::filteredNodes<UsingForDirective>(sourceUnit->nodes());
	else if (auto const* contract = dynamic_cast<ContractDefinition const*>(&_scope))
		usingForDirectives +=
			contract->usingForDirectives() +
			ASTNode::filteredNodes<UsingForDirective>(contract->sourceUnit().nodes());
	else
		solAssert(false, "");

	// Normalise data location of type.
	DataLocation typeLocation = DataLocation::Storage;
	if (auto refType = dynamic_cast<ReferenceType const*>(&_type))
		typeLocation = refType->location();

	set<Declaration const*> seenFunctions;
	MemberList::MemberMap members;

	for (UsingForDirective const* ufd: usingForDirectives)
	{
		// Convert both types to pointers for comparison to see if the `using for`
		// directive applies.
		// Further down, we check more detailed for each function if `_type` is
		// convertible to the function parameter type.
		if (ufd->typeName() &&
			*TypeProvider::withLocationIfReference(typeLocation, &_type, true) !=
			*TypeProvider::withLocationIfReference(
				typeLocation,
				ufd->typeName()->annotation().type,
				true
			)
		)
			continue;
		auto const& library = dynamic_cast<ContractDefinition const&>(
			*ufd->libraryName().annotation().referencedDeclaration
		);
		for (FunctionDefinition const* function: library.definedFunctions())
		{
			if (!function->isOrdinary() || !function->isVisibleAsLibraryMember() || seenFunctions.count(function))
				continue;
			seenFunctions.insert(function);
			if (function->parameters().empty())
				continue;
			FunctionTypePointer fun =
				dynamic_cast<FunctionType const&>(*function->typeViaContractName()).asBoundFunction();
			if (_type.isImplicitlyConvertibleTo(*fun->selfType()))
				members.emplace_back(function->name(), fun, function);
		}
	}

	return members;
}

AddressType::AddressType(StateMutability _stateMutability):
	m_stateMutability(_stateMutability)
{
	solAssert(m_stateMutability == StateMutability::Payable || m_stateMutability == StateMutability::NonPayable, "");
}

string AddressType::richIdentifier() const
{
	if (m_stateMutability == StateMutability::Payable)
		return "t_address_payable";
	else
		return "t_address";
}

BoolResult AddressType::isImplicitlyConvertibleTo(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	AddressType const& other = dynamic_cast<AddressType const&>(_other);

	return other.m_stateMutability <= m_stateMutability;
}

BoolResult AddressType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if ((_convertTo.category() == category()) || isImplicitlyConvertibleTo(_convertTo))
		return true;
	else if (auto const* contractType = dynamic_cast<ContractType const*>(&_convertTo))
		return (m_stateMutability >= StateMutability::Payable) || !contractType->isPayable();
	else if (m_stateMutability == StateMutability::NonPayable)
	{
		if (auto integerType = dynamic_cast<IntegerType const*>(&_convertTo))
			return (!integerType->isSigned() && integerType->numBits() == 168); // Solidity++: 168-bit address
		else if (auto fixedBytesType = dynamic_cast<FixedBytesType const*>(&_convertTo))
			return (fixedBytesType->numBytes() == 21);  // Solidity++: 168-bit address
	}

	return false;
}

string AddressType::toString(bool) const
{
	if (m_stateMutability == StateMutability::Payable)
		return "address payable";
	else
		return "address";
}

string AddressType::canonicalName() const
{
	return "address";
}

// Solidity++:
u256 AddressType::literalValue(Literal const* _literal) const
{
	solAssert(_literal, "");
	solAssert(_literal->value().substr(0, 5) == "vite_", "Vite address must begin with vite_");

	return u256(_literal->getViteAddressHex());
}

TypeResult AddressType::unaryOperatorResult(Token _operator) const
{
	return _operator == Token::Delete ? TypeProvider::emptyTuple() : nullptr;
}


TypeResult AddressType::binaryOperatorResult(Token _operator, Type const* _other) const
{
	if (!TokenTraits::isCompareOp(_operator))
		return TypeResult::err("Arithmetic operations on addresses are not supported. Convert to integer first before using them.");

	return Type::commonType(this, _other);
}

bool AddressType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	AddressType const& other = dynamic_cast<AddressType const&>(_other);
	return other.m_stateMutability == m_stateMutability;
}

MemberList::MemberMap AddressType::nativeMembers(ASTNode const*) const
{
	MemberList::MemberMap members = {
		{"balance", TypeProvider::uint256()},
		{"code", TypeProvider::array(DataLocation::Memory)},
		{"codehash",  TypeProvider::fixedBytes(32)},
		{"call", TypeProvider::function(strings{"bytes memory"}, strings{"bool", "bytes memory"}, FunctionType::Kind::BareCall, false, StateMutability::Payable)},
		{"callcode", TypeProvider::function(strings{"bytes memory"}, strings{"bool", "bytes memory"}, FunctionType::Kind::BareCallCode, false, StateMutability::Payable)},
		{"delegatecall", TypeProvider::function(strings{"bytes memory"}, strings{"bool", "bytes memory"}, FunctionType::Kind::BareDelegateCall, false, StateMutability::NonPayable)},
		{"staticcall", TypeProvider::function(strings{"bytes memory"}, strings{"bool", "bytes memory"}, FunctionType::Kind::BareStaticCall, false, StateMutability::View)}
	};
	if (m_stateMutability == StateMutability::Payable)
	{
		// Solidity++: redefine address.send()
		members.emplace_back(MemberList::Member{"send", TypeProvider::function(strings{"uint"}, strings{"bool"}, FunctionType::Kind::Send, false, StateMutability::NonPayable)});
		// members.emplace_back(MemberList::Member{"send", TypeProvider::function(strings{"message"}, strings{}, FunctionType::Kind::Send, false, StateMutability::Payable)});
        // Solidity++: redefine address.transfer()
		members.emplace_back(MemberList::Member{"transfer", TypeProvider::function(strings{"tokenId", "uint"}, strings(), FunctionType::Kind::Transfer, false, StateMutability::NonPayable)});
	}
	return members;
}

// Solidity++: ViteTokenIdType definitions
string ViteTokenIdType::richIdentifier() const
{
	return "t_tokenId";
}

string ViteTokenIdType::toString(bool) const
{
	return "tokenId";
}

u256 ViteTokenIdType::literalValue(Literal const* _literal) const
{
	solAssert(_literal, "");
	solAssert(_literal->value().substr(0, 4) == "tti_", "Vite Token Id must begin with tti_");

	return u256(_literal->getViteTokenIdHex());
}

namespace
{

bool isValidShiftAndAmountType(Token _operator, Type const& _shiftAmountType)
{
	// Disable >>> here.
	if (_operator == Token::SHR)
		return false;
	else if (IntegerType const* otherInt = dynamic_cast<decltype(otherInt)>(&_shiftAmountType))
		return !otherInt->isSigned();
	else if (RationalNumberType const* otherRat = dynamic_cast<decltype(otherRat)>(&_shiftAmountType))
		return !otherRat->isFractional() && otherRat->integerType() && !otherRat->integerType()->isSigned();
	else
		return false;
}

}

IntegerType::IntegerType(unsigned _bits, IntegerType::Modifier _modifier):
	m_bits(_bits), m_modifier(_modifier)
{
	solAssert(
		m_bits > 0 && m_bits <= 256 && m_bits % 8 == 0,
		"Invalid bit number for integer type: " + util::toString(m_bits)
	);
}

string IntegerType::richIdentifier() const
{
	return "t_" + string(isSigned() ? "" : "u") + "int" + to_string(numBits());
}

BoolResult IntegerType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo.category() == category())
	{
		IntegerType const& convertTo = dynamic_cast<IntegerType const&>(_convertTo);
		// disallowing unsigned to signed conversion of different bits
		if (isSigned() != convertTo.isSigned())
			return false;
		else if (convertTo.m_bits < m_bits)
			return false;
		else
			return true;
	}
	else if (_convertTo.category() == Category::FixedPoint)
	{
		FixedPointType const& convertTo = dynamic_cast<FixedPointType const&>(_convertTo);
		return maxValue() <= convertTo.maxIntegerValue() && minValue() >= convertTo.minIntegerValue();
	}
	else
		return false;
}

BoolResult IntegerType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (isImplicitlyConvertibleTo(_convertTo))
		return true;
	else if (auto integerType = dynamic_cast<IntegerType const*>(&_convertTo))
		return (numBits() == integerType->numBits()) || (isSigned() == integerType->isSigned());
	else if (auto addressType = dynamic_cast<AddressType const*>(&_convertTo))
		return
			(addressType->stateMutability() != StateMutability::Payable) &&
			!isSigned() &&
			(numBits() == 168);  // Solidity++: 168-bit address
	else if (auto fixedBytesType = dynamic_cast<FixedBytesType const*>(&_convertTo))
		return (!isSigned() && (numBits() == fixedBytesType->numBytes() * 8));
	else if (dynamic_cast<EnumType const*>(&_convertTo))
		return true;
	else if (auto fixedPointType = dynamic_cast<FixedPointType const*>(&_convertTo))
		return (isSigned() == fixedPointType->isSigned()) && (numBits() == fixedPointType->numBits());

	return false;
}

TypeResult IntegerType::unaryOperatorResult(Token _operator) const
{
	// "delete" is ok for all integer types
	if (_operator == Token::Delete)
		return TypeResult{TypeProvider::emptyTuple()};
	// unary negation only on signed types
	else if (_operator == Token::Sub)
		return isSigned() ? TypeResult{this} : TypeResult::err("Unary negation is only allowed for signed integers.");
	else if (_operator == Token::Inc || _operator == Token::Dec || _operator == Token::BitNot)
		return TypeResult{this};
	else
		return TypeResult::err("");
}

bool IntegerType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	IntegerType const& other = dynamic_cast<IntegerType const&>(_other);
	return other.m_bits == m_bits && other.m_modifier == m_modifier;
}

string IntegerType::toString(bool) const
{
	string prefix = isSigned() ? "int" : "uint";
	return prefix + util::toString(m_bits);
}

u256 IntegerType::min() const
{
	if (isSigned())
		return s2u(s256(minValue()));
	else
		return u256(minValue());
}

u256 IntegerType::max() const
{
	if (isSigned())
		return s2u(s256(maxValue()));
	else
		return u256(maxValue());
}

bigint IntegerType::minValue() const
{
	if (isSigned())
		return -(bigint(1) << (m_bits - 1));
	else
		return bigint(0);
}

bigint IntegerType::maxValue() const
{
	if (isSigned())
		return (bigint(1) << (m_bits - 1)) - 1;
	else
		return (bigint(1) << m_bits) - 1;
}

TypeResult IntegerType::binaryOperatorResult(Token _operator, Type const* _other) const
{
	if (
		_other->category() != Category::RationalNumber &&
		_other->category() != Category::FixedPoint &&
		_other->category() != category()
	)
		return nullptr;
	if (TokenTraits::isShiftOp(_operator))
	{
		// Shifts are not symmetric with respect to the type
		if (isValidShiftAndAmountType(_operator, *_other))
			return this;
		else
			return nullptr;
	}
	else if (Token::Exp == _operator)
	{
		if (auto otherIntType = dynamic_cast<IntegerType const*>(_other))
		{
			if (otherIntType->isSigned())
				return TypeResult::err("Exponentiation power is not allowed to be a signed integer type.");
		}
		else if (dynamic_cast<FixedPointType const*>(_other))
			return nullptr;
		else if (auto rationalNumberType = dynamic_cast<RationalNumberType const*>(_other))
		{
			if (rationalNumberType->isFractional())
				return TypeResult::err("Exponent is fractional.");
			if (!rationalNumberType->integerType())
				return TypeResult::err("Exponent too large.");
			if (rationalNumberType->isNegative())
				return TypeResult::err("Exponentiation power is not allowed to be a negative integer literal.");
		}
		return this;
	}

	auto commonType = Type::commonType(this, _other); //might be an integer or fixed point
	if (!commonType)
		return nullptr;

	// All integer types can be compared
	if (TokenTraits::isCompareOp(_operator))
		return commonType;
	if (TokenTraits::isBooleanOp(_operator))
		return nullptr;
	return commonType;
}

FixedPointType::FixedPointType(unsigned _totalBits, unsigned _fractionalDigits, FixedPointType::Modifier _modifier):
	m_totalBits(_totalBits), m_fractionalDigits(_fractionalDigits), m_modifier(_modifier)
{
	solAssert(
		8 <= m_totalBits && m_totalBits <= 256 && m_totalBits % 8 == 0 && m_fractionalDigits <= 80,
		"Invalid bit number(s) for fixed type: " +
		util::toString(_totalBits) + "x" + util::toString(_fractionalDigits)
	);
}

string FixedPointType::richIdentifier() const
{
	return "t_" + string(isSigned() ? "" : "u") + "fixed" + to_string(m_totalBits) + "x" + to_string(m_fractionalDigits);
}

BoolResult FixedPointType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo.category() == category())
	{
		FixedPointType const& convertTo = dynamic_cast<FixedPointType const&>(_convertTo);
		if (convertTo.fractionalDigits() < m_fractionalDigits)
			return BoolResult::err("Too many fractional digits.");
		if (convertTo.numBits() < m_totalBits)
			return false;
		else
			return convertTo.maxIntegerValue() >= maxIntegerValue() && convertTo.minIntegerValue() <= minIntegerValue();
	}
	return false;
}

BoolResult FixedPointType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	return _convertTo.category() == category() || _convertTo.category() == Category::Integer;
}

TypeResult FixedPointType::unaryOperatorResult(Token _operator) const
{
	switch (_operator)
	{
	case Token::Delete:
		// "delete" is ok for all fixed types
		return TypeResult{TypeProvider::emptyTuple()};
	case Token::Add:
	case Token::Sub:
	case Token::Inc:
	case Token::Dec:
		// for fixed, we allow +, -, ++ and --
		return this;
	default:
		return nullptr;
	}
}

bool FixedPointType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	FixedPointType const& other = dynamic_cast<FixedPointType const&>(_other);
	return other.m_totalBits == m_totalBits && other.m_fractionalDigits == m_fractionalDigits && other.m_modifier == m_modifier;
}

string FixedPointType::toString(bool) const
{
	string prefix = isSigned() ? "fixed" : "ufixed";
	return prefix + util::toString(m_totalBits) + "x" + util::toString(m_fractionalDigits);
}

bigint FixedPointType::maxIntegerValue() const
{
	bigint maxValue = (bigint(1) << (m_totalBits - (isSigned() ? 1 : 0))) - 1;
	return maxValue / boost::multiprecision::pow(bigint(10), m_fractionalDigits);
}

bigint FixedPointType::minIntegerValue() const
{
	if (isSigned())
	{
		bigint minValue = -(bigint(1) << (m_totalBits - (isSigned() ? 1 : 0)));
		return minValue / boost::multiprecision::pow(bigint(10), m_fractionalDigits);
	}
	else
		return bigint(0);
}

TypeResult FixedPointType::binaryOperatorResult(Token _operator, Type const* _other) const
{
	auto commonType = Type::commonType(this, _other);

	if (!commonType)
		return nullptr;

	// All fixed types can be compared
	if (TokenTraits::isCompareOp(_operator))
		return commonType;
	if (TokenTraits::isBitOp(_operator) || TokenTraits::isBooleanOp(_operator) || _operator == Token::Exp)
		return nullptr;
	return commonType;
}

IntegerType const* FixedPointType::asIntegerType() const
{
	return TypeProvider::integer(numBits(), isSigned() ? IntegerType::Modifier::Signed : IntegerType::Modifier::Unsigned);
}

tuple<bool, rational> RationalNumberType::parseRational(string const& _value)
{
	rational value;
	try
	{
		auto radixPoint = find(_value.begin(), _value.end(), '.');

		if (radixPoint != _value.end())
		{
			if (
				!all_of(radixPoint + 1, _value.end(), ::isdigit) ||
				!all_of(_value.begin(), radixPoint, ::isdigit)
			)
				return make_tuple(false, rational(0));

			// Only decimal notation allowed here, leading zeros would switch to octal.
			auto fractionalBegin = find_if_not(
				radixPoint + 1,
				_value.end(),
				[](char const& a) { return a == '0'; }
			);

			rational numerator;
			rational denominator(1);

			denominator = bigint(string(fractionalBegin, _value.end()));
			denominator /= boost::multiprecision::pow(
				bigint(10),
				static_cast<unsigned>(distance(radixPoint + 1, _value.end()))
			);
			numerator = bigint(string(_value.begin(), radixPoint));
			value = numerator + denominator;
		}
		else
			value = bigint(_value);
		return make_tuple(true, value);
	}
	catch (...)
	{
		return make_tuple(false, rational(0));
	}
}

tuple<bool, rational> RationalNumberType::isValidLiteral(Literal const& _literal)
{
	rational value;
	try
	{
		ASTString valueString = _literal.valueWithoutUnderscores();

		auto expPoint = find(valueString.begin(), valueString.end(), 'e');
		if (expPoint == valueString.end())
			expPoint = find(valueString.begin(), valueString.end(), 'E');

		if (boost::starts_with(valueString, "0x"))
		{
			// process as hex
			value = bigint(valueString);
		}
		else if (expPoint != valueString.end())
		{
			// Parse mantissa and exponent. Checks numeric limit.
			tuple<bool, rational> mantissa = parseRational(string(valueString.begin(), expPoint));

			if (!get<0>(mantissa))
				return make_tuple(false, rational(0));
			value = get<1>(mantissa);

			// 0E... is always zero.
			if (value == 0)
				return make_tuple(true, rational(0));

			bigint exp = bigint(string(expPoint + 1, valueString.end()));

			if (exp > numeric_limits<int32_t>::max() || exp < numeric_limits<int32_t>::min())
				return make_tuple(false, rational(0));

			uint32_t expAbs = bigint(abs(exp)).convert_to<uint32_t>();

			if (exp < 0)
			{
				if (!fitsPrecisionBase10(abs(value.denominator()), expAbs))
					return make_tuple(false, rational(0));
				value /= boost::multiprecision::pow(
					bigint(10),
					expAbs
				);
			}
			else if (exp > 0)
			{
				if (!fitsPrecisionBase10(abs(value.numerator()), expAbs))
					return make_tuple(false, rational(0));
				value *= boost::multiprecision::pow(
					bigint(10),
					expAbs
				);
			}
		}
		else
		{
			// parse as rational number
			tuple<bool, rational> tmp = parseRational(valueString);
			if (!get<0>(tmp))
				return tmp;
			value = get<1>(tmp);
		}
	}
	catch (...)
	{
		return make_tuple(false, rational(0));
	}

    switch (_literal.subDenomination())
    {
        case Literal::SubDenomination::None:
        case Literal::SubDenomination::Wei:
        case Literal::SubDenomination::Second:
            break;
        case Literal::SubDenomination::Gwei:
            value *= bigint("1000000000");
            break;
        case Literal::SubDenomination::Ether:
            value *= bigint("1000000000000000000");
            break;
        case Literal::SubDenomination::Minute:
            value *= bigint("60");
            break;
        case Literal::SubDenomination::Hour:
            value *= bigint("3600");
            break;
        case Literal::SubDenomination::Day:
            value *= bigint("86400");
            break;
        case Literal::SubDenomination::Week:
            value *= bigint("604800");
            break;
        case Literal::SubDenomination::Year:
            value *= bigint("31536000");
            break;
        case Literal::SubDenomination::Attov:
            break;
        case Literal::SubDenomination::Vite:
            value *= bigint("1000000000000000000");
    }

	return make_tuple(true, value);
}

BoolResult RationalNumberType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	switch (_convertTo.category())
	{
	case Category::Integer:
	{
		if (isFractional())
			return false;
		IntegerType const& targetType = dynamic_cast<IntegerType const&>(_convertTo);
		return fitsIntegerType(m_value.numerator(), targetType);
	}
	case Category::FixedPoint:
	{
		FixedPointType const& targetType = dynamic_cast<FixedPointType const&>(_convertTo);
		// Store a negative number into an unsigned.
		if (isNegative() && !targetType.isSigned())
			return false;
		if (!isFractional())
			return (targetType.minIntegerValue() <= m_value) && (m_value <= targetType.maxIntegerValue());
		rational value = m_value * pow(bigint(10), targetType.fractionalDigits());
		// Need explicit conversion since truncation will occur.
		if (value.denominator() != 1)
			return false;
		return fitsIntoBits(value.numerator(), targetType.numBits(), targetType.isSigned());
	}
	case Category::FixedBytes:
		return (m_value == rational(0)) || (m_compatibleBytesType && *m_compatibleBytesType == _convertTo);
	default:
		return false;
	}
}

BoolResult RationalNumberType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (isImplicitlyConvertibleTo(_convertTo))
		return true;

	auto category = _convertTo.category();
	if (category == Category::FixedBytes)
		return false;
	else if (auto addressType = dynamic_cast<AddressType const*>(&_convertTo))
		return	(m_value == 0) ||
			((addressType->stateMutability() != StateMutability::Payable) &&
			!isNegative() &&
			!isFractional() &&
			integerType() &&
			(integerType()->numBits() <= 168));  // Solidity++: 168-bit address
	else if (category == Category::Integer)
		return false;
	else if (auto enumType = dynamic_cast<EnumType const*>(&_convertTo))
		if (isNegative() || isFractional() || m_value >= enumType->numberOfMembers())
			return false;

	TypePointer mobType = mobileType();
	return (mobType && mobType->isExplicitlyConvertibleTo(_convertTo));

}

TypeResult RationalNumberType::unaryOperatorResult(Token _operator) const
{
	if (optional<rational> value = ConstantEvaluator::evaluateUnaryOperator(_operator, m_value))
		return TypeResult{TypeProvider::rationalNumber(*value)};
	else
		return nullptr;
}

TypeResult RationalNumberType::binaryOperatorResult(Token _operator, Type const* _other) const
{
	if (_other->category() == Category::Integer || _other->category() == Category::FixedPoint)
	{
		if (isFractional())
			return TypeResult::err("Fractional literals not supported.");
		else if (!integerType())
			return TypeResult::err("Literal too large.");

		// Shift and exp are not symmetric, so it does not make sense to swap
		// the types as below. As an exception, we always use uint here.
		if (TokenTraits::isShiftOp(_operator))
		{
			if (!isValidShiftAndAmountType(_operator, *_other))
				return nullptr;
			return isNegative() ? TypeProvider::int256() : TypeProvider::uint256();
		}
		else if (Token::Exp == _operator)
		{
			if (auto const* otherIntType = dynamic_cast<IntegerType const*>(_other))
			{
				if (otherIntType->isSigned())
					return TypeResult::err("Exponentiation power is not allowed to be a signed integer type.");
			}
			else if (dynamic_cast<FixedPointType const*>(_other))
				return TypeResult::err("Exponent is fractional.");

			return isNegative() ? TypeProvider::int256() : TypeProvider::uint256();
		}
		else
		{
			auto commonType = Type::commonType(this, _other);
			if (!commonType)
				return nullptr;
			return commonType->binaryOperatorResult(_operator, _other);
		}
	}
	else if (_other->category() != category())
		return nullptr;

	RationalNumberType const& other = dynamic_cast<RationalNumberType const&>(*_other);
	if (TokenTraits::isCompareOp(_operator))
	{
		// Since we do not have a "BoolConstantType", we have to do the actual comparison
		// at runtime and convert to mobile typse first. Such a comparison is not a very common
		// use-case and will be optimized away.
		TypePointer thisMobile = mobileType();
		TypePointer otherMobile = other.mobileType();
		if (!thisMobile || !otherMobile)
			return nullptr;
		return thisMobile->binaryOperatorResult(_operator, otherMobile);
	}
	else if (optional<rational> value = ConstantEvaluator::evaluateBinaryOperator(_operator, m_value, other.m_value))
	{
		// verify that numerator and denominator fit into 4096 bit after every operation
		if (value->numerator() != 0 && max(boost::multiprecision::msb(abs(value->numerator())), boost::multiprecision::msb(abs(value->denominator()))) > 4096)
			return TypeResult::err("Precision of rational constants is limited to 4096 bits.");

		return TypeResult{TypeProvider::rationalNumber(*value)};
	}
	else
		return nullptr;
}

string RationalNumberType::richIdentifier() const
{
	// rational seemingly will put the sign always on the numerator,
	// but let just make it deterministic here.
	bigint numerator = abs(m_value.numerator());
	bigint denominator = abs(m_value.denominator());
	if (m_value < 0)
		return "t_rational_minus_" + numerator.str() + "_by_" + denominator.str();
	else
		return "t_rational_" + numerator.str() + "_by_" + denominator.str();
}

bool RationalNumberType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	RationalNumberType const& other = dynamic_cast<RationalNumberType const&>(_other);
	return m_value == other.m_value;
}

string RationalNumberType::bigintToReadableString(bigint const& _num)
{
	string str = _num.str();
	if (str.size() > 32)
	{
		size_t omitted = str.size() - 8;
		str = str.substr(0, 4) + "...(" + to_string(omitted) + " digits omitted)..." + str.substr(str.size() - 4, 4);
	}
	return str;
}

string RationalNumberType::toString(bool) const
{
	if (!isFractional())
		return "int_const " + bigintToReadableString(m_value.numerator());

	string numerator = bigintToReadableString(m_value.numerator());
	string denominator = bigintToReadableString(m_value.denominator());
	return "rational_const " + numerator + " / " + denominator;
}

u256 RationalNumberType::literalValue(Literal const*) const
{
	// We ignore the literal and hope that the type was correctly determined to represent
	// its value.

	u256 value;
	bigint shiftedValue;

	if (!isFractional())
		shiftedValue = m_value.numerator();
	else
	{
		auto fixed = fixedPointType();
		solAssert(fixed, "Rational number cannot be represented as fixed point type.");
		unsigned fractionalDigits = fixed->fractionalDigits();
		shiftedValue = m_value.numerator() * boost::multiprecision::pow(bigint(10), fractionalDigits) / m_value.denominator();
	}

	// we ignore the literal and hope that the type was correctly determined
	solAssert(shiftedValue <= u256(-1), "Number constant too large.");
	solAssert(shiftedValue >= -(bigint(1) << 255), "Number constant too small.");

	if (m_value >= rational(0))
		value = u256(shiftedValue);
	else
		value = s2u(s256(shiftedValue));
	return value;
}

TypePointer RationalNumberType::mobileType() const
{
	if (!isFractional())
		return integerType();
	else
		return fixedPointType();
}

IntegerType const* RationalNumberType::integerType() const
{
	solAssert(!isFractional(), "integerType() called for fractional number.");
	bigint value = m_value.numerator();
	bool negative = (value < 0);
	if (negative) // convert to positive number of same bit requirements
		value = ((0 - value) - 1) << 1;
	if (value > u256(-1))
		return nullptr;
	else
		return TypeProvider::integer(
			max(util::bytesRequired(value), 1u) * 8,
			negative ? IntegerType::Modifier::Signed : IntegerType::Modifier::Unsigned
		);
}

FixedPointType const* RationalNumberType::fixedPointType() const
{
	bool negative = (m_value < 0);
	unsigned fractionalDigits = 0;
	rational value = abs(m_value); // We care about the sign later.
	rational maxValue = negative ?
		rational(bigint(1) << 255, 1):
		rational((bigint(1) << 256) - 1, 1);

	while (value * 10 <= maxValue && value.denominator() != 1 && fractionalDigits < 80)
	{
		value *= 10;
		fractionalDigits++;
	}

	if (value > maxValue)
		return nullptr;

	// This means we round towards zero for positive and negative values.
	bigint v = value.numerator() / value.denominator();

	if (negative && v != 0)
		// modify value to satisfy bit requirements for negative numbers:
		// add one bit for sign and decrement because negative numbers can be larger
		v = (v - 1) << 1;

	if (v > u256(-1))
		return nullptr;

	unsigned totalBits = max(util::bytesRequired(v), 1u) * 8;
	solAssert(totalBits <= 256, "");

	return TypeProvider::fixedPoint(
		totalBits, fractionalDigits,
		negative ? FixedPointType::Modifier::Signed : FixedPointType::Modifier::Unsigned
	);
}

StringLiteralType::StringLiteralType(Literal const& _literal):
	m_value(_literal.value())
{
}

StringLiteralType::StringLiteralType(string _value):
	m_value{std::move(_value)}
{
}

BoolResult StringLiteralType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (auto fixedBytes = dynamic_cast<FixedBytesType const*>(&_convertTo))
	{
		if (static_cast<size_t>(fixedBytes->numBytes()) < m_value.size())
			return BoolResult::err("Literal is larger than the type.");
		return true;
	}
	else if (auto arrayType = dynamic_cast<ArrayType const*>(&_convertTo))
	{
		size_t invalidSequence;
		if (arrayType->isString() && !util::validateUTF8(value(), invalidSequence))
			return BoolResult::err(
				"Contains invalid UTF-8 sequence at position " +
				util::toString(invalidSequence) +
				"."
			);
		return
			arrayType->location() != DataLocation::CallData &&
			arrayType->isByteArray() &&
			!(arrayType->dataStoredIn(DataLocation::Storage) && arrayType->isPointer());
	}
	else
		return false;
}

string StringLiteralType::richIdentifier() const
{
	// Since we have to return a valid identifier and the string itself may contain
	// anything, we hash it.
	// Solidity++: keccak256 -> blake2b
	return "t_stringliteral_" + util::toHex(util::blake2b(m_value).asBytes());
}

bool StringLiteralType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	return m_value == dynamic_cast<StringLiteralType const&>(_other).m_value;
}

std::string StringLiteralType::toString(bool) const
{
	auto isPrintableASCII = [](string const& s)
	{
		for (auto c: s)
		{
			if (static_cast<unsigned>(c) <= 0x1f || static_cast<unsigned>(c) >= 0x7f)
				return false;
		}
		return true;
	};

	return isPrintableASCII(m_value) ?
		("literal_string \"" + m_value + "\"") :
		("literal_string hex\"" + util::toHex(util::asBytes(m_value)) + "\"");
}

TypePointer StringLiteralType::mobileType() const
{
	return TypeProvider::stringMemory();
}

FixedBytesType::FixedBytesType(unsigned _bytes): m_bytes(_bytes)
{
	solAssert(
		m_bytes > 0 && m_bytes <= 32,
		"Invalid byte number for fixed bytes type: " + util::toString(m_bytes)
	);
}

BoolResult FixedBytesType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo.category() != category())
		return false;
	FixedBytesType const& convertTo = dynamic_cast<FixedBytesType const&>(_convertTo);
	return convertTo.m_bytes >= m_bytes;
}

BoolResult FixedBytesType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo.category() == category())
		return true;
	else if (auto integerType = dynamic_cast<IntegerType const*>(&_convertTo))
		return (!integerType->isSigned() && integerType->numBits() == numBytes() * 8);
	else if (auto addressType = dynamic_cast<AddressType const*>(&_convertTo))
		return
			(addressType->stateMutability() != StateMutability::Payable) &&
			(numBytes() == 21);  // Solidity++: 168-bit address
	else if (auto fixedPointType = dynamic_cast<FixedPointType const*>(&_convertTo))
		return fixedPointType->numBits() == numBytes() * 8;

	return false;
}

TypeResult FixedBytesType::unaryOperatorResult(Token _operator) const
{
	// "delete" and "~" is okay for FixedBytesType
	if (_operator == Token::Delete)
		return TypeResult{TypeProvider::emptyTuple()};
	else if (_operator == Token::BitNot)
		return this;

	return nullptr;
}

TypeResult FixedBytesType::binaryOperatorResult(Token _operator, Type const* _other) const
{
	if (TokenTraits::isShiftOp(_operator))
	{
		if (isValidShiftAndAmountType(_operator, *_other))
			return this;
		else
			return nullptr;
	}

	auto commonType = dynamic_cast<FixedBytesType const*>(Type::commonType(this, _other));
	if (!commonType)
		return nullptr;

	// FixedBytes can be compared and have bitwise operators applied to them
	if (TokenTraits::isCompareOp(_operator) || TokenTraits::isBitOp(_operator))
		return TypeResult(commonType);

	return nullptr;
}

MemberList::MemberMap FixedBytesType::nativeMembers(ASTNode const*) const
{
	return MemberList::MemberMap{MemberList::Member{"length", TypeProvider::uint(8)}};
}

string FixedBytesType::richIdentifier() const
{
	return "t_bytes" + to_string(m_bytes);
}

bool FixedBytesType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	FixedBytesType const& other = dynamic_cast<FixedBytesType const&>(_other);
	return other.m_bytes == m_bytes;
}

u256 BoolType::literalValue(Literal const* _literal) const
{
	solAssert(_literal, "");
	if (_literal->token() == Token::TrueLiteral)
		return u256(1);
	else if (_literal->token() == Token::FalseLiteral)
		return u256(0);
	else
		solAssert(false, "Bool type constructed from non-boolean literal.");
}

TypeResult BoolType::unaryOperatorResult(Token _operator) const
{
	if (_operator == Token::Delete)
		return TypeProvider::emptyTuple();
	else if (_operator == Token::Not)
		return this;
	else
		return nullptr;
}

TypeResult BoolType::binaryOperatorResult(Token _operator, Type const* _other) const
{
	if (category() != _other->category())
		return nullptr;
	if (_operator == Token::Equal || _operator == Token::NotEqual || _operator == Token::And || _operator == Token::Or)
		return _other;
	else
		return nullptr;
}

Type const* ContractType::encodingType() const
{
	if (isSuper())
		return nullptr;

	if (isPayable())
		return TypeProvider::payableAddress();
	else
		return TypeProvider::address();
}

BoolResult ContractType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (m_super)
		return false;

	if (*this == _convertTo)
		return true;
	if (_convertTo.category() == Category::Contract)
	{
		auto const& targetContractType = dynamic_cast<ContractType const&>(_convertTo);
		if (targetContractType.isSuper())
			return false;

		auto const& bases = contractDefinition().annotation().linearizedBaseContracts;
		return find(
			bases.begin(),
			bases.end(),
			&targetContractType.contractDefinition()
		) != bases.end();
	}
	return false;
}

BoolResult ContractType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (m_super)
		return false;

	if (auto const* addressType = dynamic_cast<AddressType const*>(&_convertTo))
		return isPayable() || (addressType->stateMutability() < StateMutability::Payable);

	return isImplicitlyConvertibleTo(_convertTo);
}

bool ContractType::isPayable() const
{
	auto receiveFunction = m_contract.receiveFunction();
	auto fallbackFunction = m_contract.fallbackFunction();
	return receiveFunction || (fallbackFunction && fallbackFunction->isPayable());
}

TypeResult ContractType::unaryOperatorResult(Token _operator) const
{
	if (isSuper())
		return nullptr;
	else if (_operator == Token::Delete)
		return TypeProvider::emptyTuple();
	else
		return nullptr;
}

vector<Type const*> CompositeType::fullDecomposition() const
{
	vector<Type const*> res = {this};
	unordered_set<string> seen = {richIdentifier()};
	for (size_t k = 0; k < res.size(); ++k)
		if (auto composite = dynamic_cast<CompositeType const*>(res[k]))
			for (Type const* next: composite->decomposition())
				if (seen.count(next->richIdentifier()) == 0)
				{
					seen.insert(next->richIdentifier());
					res.push_back(next);
				}
	return res;
}

Type const* ReferenceType::withLocation(DataLocation _location, bool _isPointer) const
{
	return TypeProvider::withLocation(this, _location, _isPointer);
}

TypeResult ReferenceType::unaryOperatorResult(Token _operator) const
{
	if (_operator != Token::Delete)
		return nullptr;
	// delete can be used on everything except calldata references or storage pointers
	// (storage references are ok)
	switch (location())
	{
	case DataLocation::CallData:
		return nullptr;
	case DataLocation::Memory:
		return TypeProvider::emptyTuple();
	case DataLocation::Storage:
		return isPointer() ? nullptr : TypeProvider::emptyTuple();
	}
	return nullptr;
}

bool ReferenceType::isPointer() const
{
	if (m_location == DataLocation::Storage)
		return m_isPointer;
	else
		return true;
}

TypePointer ReferenceType::copyForLocationIfReference(Type const* _type) const
{
	return TypeProvider::withLocationIfReference(m_location, _type);
}

string ReferenceType::stringForReferencePart() const
{
	switch (m_location)
	{
	case DataLocation::Storage:
		return string("storage ") + (isPointer() ? "pointer" : "ref");
	case DataLocation::CallData:
		return "calldata";
	case DataLocation::Memory:
		return "memory";
	}
	solAssert(false, "");
	return "";
}

string ReferenceType::identifierLocationSuffix() const
{
	string id;
	switch (location())
	{
	case DataLocation::Storage:
		id += "_storage";
		break;
	case DataLocation::Memory:
		id += "_memory";
		break;
	case DataLocation::CallData:
		id += "_calldata";
		break;
	}
	if (isPointer())
		id += "_ptr";
	return id;
}

ArrayType::ArrayType(DataLocation _location, bool _isString):
	ReferenceType(_location),
	m_arrayKind(_isString ? ArrayKind::String : ArrayKind::Bytes),
	m_baseType{TypeProvider::byte()}
{
}

void ArrayType::clearCache() const
{
	Type::clearCache();

	m_interfaceType.reset();
	m_interfaceType_library.reset();
}

BoolResult ArrayType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo.category() != category())
		return false;
	auto& convertTo = dynamic_cast<ArrayType const&>(_convertTo);
	if (convertTo.isByteArray() != isByteArray() || convertTo.isString() != isString())
		return false;
	// memory/calldata to storage can be converted, but only to a direct storage reference
	if (convertTo.location() == DataLocation::Storage && location() != DataLocation::Storage && convertTo.isPointer())
		return false;
	if (convertTo.location() == DataLocation::CallData && location() != convertTo.location())
		return false;
	if (convertTo.location() == DataLocation::Storage && !convertTo.isPointer())
	{
		// Less restrictive conversion, since we need to copy anyway.
		if (!baseType()->isImplicitlyConvertibleTo(*convertTo.baseType()))
			return false;
		if (convertTo.isDynamicallySized())
			return true;
		return !isDynamicallySized() && convertTo.length() >= length();
	}
	else
	{
		// Conversion to storage pointer or to memory, we de not copy element-for-element here, so
		// require that the base type is the same, not only convertible.
		// This disallows assignment of nested dynamic arrays from storage to memory for now.
		if (
			*TypeProvider::withLocationIfReference(location(), baseType()) !=
			*TypeProvider::withLocationIfReference(location(), convertTo.baseType())
		)
			return false;
		if (isDynamicallySized() != convertTo.isDynamicallySized())
			return false;
		// We also require that the size is the same.
		if (!isDynamicallySized() && length() != convertTo.length())
			return false;
		return true;
	}
}

BoolResult ArrayType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (isImplicitlyConvertibleTo(_convertTo))
		return true;
	// allow conversion bytes <-> string
	if (_convertTo.category() != category())
		return false;
	auto& convertTo = dynamic_cast<ArrayType const&>(_convertTo);
	if (convertTo.location() != location())
		return false;
	if (!isByteArray() || !convertTo.isByteArray())
		return false;
	return true;
}

string ArrayType::richIdentifier() const
{
	string id;
	if (isString())
		id = "t_string";
	else if (isByteArray())
		id = "t_bytes";
	else
	{
		id = "t_array";
		id += identifierList(baseType());
		if (isDynamicallySized())
			id += "dyn";
		else
			id += length().str();
	}
	id += identifierLocationSuffix();

	return id;
}

bool ArrayType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	ArrayType const& other = dynamic_cast<ArrayType const&>(_other);
	if (
		!ReferenceType::operator==(other) ||
		other.isByteArray() != isByteArray() ||
		other.isString() != isString() ||
		other.isDynamicallySized() != isDynamicallySized()
	)
		return false;
	if (*other.baseType() != *baseType())
		return false;
	return isDynamicallySized() || length() == other.length();
}

BoolResult ArrayType::validForLocation(DataLocation _loc) const
{
	if (auto arrayBaseType = dynamic_cast<ArrayType const*>(baseType()))
	{
		BoolResult result = arrayBaseType->validForLocation(_loc);
		if (!result)
			return result;
	}
	if (isDynamicallySized())
		return true;
	switch (_loc)
	{
		case DataLocation::Memory:
		{
			bigint size = bigint(length());
			auto type = m_baseType;
			while (auto arrayType = dynamic_cast<ArrayType const*>(type))
			{
				if (arrayType->isDynamicallySized())
					break;
				else
				{
					size *= arrayType->length();
					type = arrayType->baseType();
				}
			}
			if (type->isDynamicallySized())
				size *= type->memoryHeadSize();
			else
				size *= type->memoryDataSize();
			if (size >= numeric_limits<unsigned>::max())
				return BoolResult::err("Type too large for memory.");
			break;
		}
		case DataLocation::CallData:
		{
			if (unlimitedStaticCalldataSize(true) >= numeric_limits<unsigned>::max())
				return BoolResult::err("Type too large for calldata.");
			break;
		}
		case DataLocation::Storage:
			if (storageSizeUpperBound() >= bigint(1) << 256)
				return BoolResult::err("Type too large for storage.");
			break;
	}
	return true;
}

bigint ArrayType::unlimitedStaticCalldataSize(bool _padded) const
{
	solAssert(!isDynamicallySized(), "");
	bigint size = bigint(length()) * calldataStride();
	if (_padded)
		size = ((size + 31) / 32) * 32;
	return size;
}

unsigned ArrayType::calldataEncodedSize(bool _padded) const
{
	solAssert(!isDynamicallyEncoded(), "");
	bigint size = unlimitedStaticCalldataSize(_padded);
	solAssert(size <= numeric_limits<unsigned>::max(), "Array size does not fit unsigned.");
	return unsigned(size);
}

unsigned ArrayType::calldataEncodedTailSize() const
{
	solAssert(isDynamicallyEncoded(), "");
	if (isDynamicallySized())
		// We do not know the dynamic length itself, but at least the uint256 containing the
		// length must still be present.
		return 32;
	bigint size = unlimitedStaticCalldataSize(false);
	solAssert(size <= numeric_limits<unsigned>::max(), "Array size does not fit unsigned.");
	return unsigned(size);
}

bool ArrayType::isDynamicallyEncoded() const
{
	return isDynamicallySized() || baseType()->isDynamicallyEncoded();
}

bigint ArrayType::storageSizeUpperBound() const
{
	if (isDynamicallySized())
		return 1;
	else
		return length() * baseType()->storageSizeUpperBound();
}

u256 ArrayType::storageSize() const
{
	if (isDynamicallySized())
		return 1;

	bigint size;
	unsigned baseBytes = baseType()->storageBytes();
	if (baseBytes == 0)
		size = 1;
	else if (baseBytes < 32)
	{
		unsigned itemsPerSlot = 32 / baseBytes;
		size = (bigint(length()) + (itemsPerSlot - 1)) / itemsPerSlot;
	}
	else
		size = bigint(length()) * baseType()->storageSize();
	solAssert(size < bigint(1) << 256, "Array too large for storage.");
	return max<u256>(1, u256(size));
}

vector<tuple<string, TypePointer>> ArrayType::makeStackItems() const
{
	switch (m_location)
	{
		case DataLocation::CallData:
			if (isDynamicallySized())
				return {std::make_tuple("offset", TypeProvider::uint256()), std::make_tuple("length", TypeProvider::uint256())};
			else
				return {std::make_tuple("offset", TypeProvider::uint256())};
		case DataLocation::Memory:
			return {std::make_tuple("mpos", TypeProvider::uint256())};
		case DataLocation::Storage:
			// byte offset inside storage value is omitted
			return {std::make_tuple("slot", TypeProvider::uint256())};
	}
	solAssert(false, "");
}

string ArrayType::toString(bool _short) const
{
	string ret;
	if (isString())
		ret = "string";
	else if (isByteArray())
		ret = "bytes";
	else
	{
		ret = baseType()->toString(_short) + "[";
		if (!isDynamicallySized())
			ret += length().str();
		ret += "]";
	}
	if (!_short)
		ret += " " + stringForReferencePart();
	return ret;
}

string ArrayType::canonicalName() const
{
	string ret;
	if (isString())
		ret = "string";
	else if (isByteArray())
		ret = "bytes";
	else
	{
		ret = baseType()->canonicalName() + "[";
		if (!isDynamicallySized())
			ret += length().str();
		ret += "]";
	}
	return ret;
}

string ArrayType::signatureInExternalFunction(bool _structsByName) const
{
	if (isByteArray())
		return canonicalName();
	else
	{
		solAssert(baseType(), "");
		return
			baseType()->signatureInExternalFunction(_structsByName) +
			"[" +
			(isDynamicallySized() ? "" : length().str()) +
			"]";
	}
}

MemberList::MemberMap ArrayType::nativeMembers(ASTNode const*) const
{
	MemberList::MemberMap members;
	if (!isString())
	{
		members.emplace_back("length", TypeProvider::uint256());
		if (isDynamicallySized() && location() == DataLocation::Storage)
		{
			members.emplace_back("push", TypeProvider::function(
				TypePointers{},
				TypePointers{baseType()},
				strings{},
				strings{string()},
				isByteArray() ? FunctionType::Kind::ByteArrayPush : FunctionType::Kind::ArrayPush
			));
			members.emplace_back("push", TypeProvider::function(
				TypePointers{baseType()},
				TypePointers{},
				strings{string()},
				strings{},
				isByteArray() ? FunctionType::Kind::ByteArrayPush : FunctionType::Kind::ArrayPush
			));
			members.emplace_back("pop", TypeProvider::function(
				TypePointers{},
				TypePointers{},
				strings{},
				strings{},
				FunctionType::Kind::ArrayPop
			));
		}
	}
	return members;
}

TypePointer ArrayType::encodingType() const
{
	if (location() == DataLocation::Storage)
		return TypeProvider::uint256();
	else
		return TypeProvider::withLocation(this, DataLocation::Memory, true);
}

TypePointer ArrayType::decodingType() const
{
	if (location() == DataLocation::Storage)
		return TypeProvider::uint256();
	else
		return this;
}

TypeResult ArrayType::interfaceType(bool _inLibrary) const
{
	if (_inLibrary && m_interfaceType_library.has_value())
		return *m_interfaceType_library;

	if (!_inLibrary && m_interfaceType.has_value())
		return *m_interfaceType;

	TypeResult result{TypePointer{}};
	TypeResult baseInterfaceType = m_baseType->interfaceType(_inLibrary);

	if (!baseInterfaceType.get())
	{
		solAssert(!baseInterfaceType.message().empty(), "Expected detailed error message!");
		result = baseInterfaceType;
	}
	else if (_inLibrary && location() == DataLocation::Storage)
		result = this;
	else if (m_arrayKind != ArrayKind::Ordinary)
		result = TypeProvider::withLocation(this, DataLocation::Memory, true);
	else if (isDynamicallySized())
		result = TypeProvider::array(DataLocation::Memory, baseInterfaceType);
	else
		result = TypeProvider::array(DataLocation::Memory, baseInterfaceType, m_length);

	if (_inLibrary)
		m_interfaceType_library = result;
	else
		m_interfaceType = result;

	return result;
}

Type const* ArrayType::finalBaseType(bool _breakIfDynamicArrayType) const
{
	Type const* finalBaseType = this;

	while (auto arrayType = dynamic_cast<ArrayType const*>(finalBaseType))
	{
		if (_breakIfDynamicArrayType && arrayType->isDynamicallySized())
			break;
		finalBaseType = arrayType->baseType();
	}

	return finalBaseType;
}

u256 ArrayType::memoryDataSize() const
{
	solAssert(!isDynamicallySized(), "");
	solAssert(m_location == DataLocation::Memory, "");
	solAssert(!isByteArray(), "");
	bigint size = bigint(m_length) * m_baseType->memoryHeadSize();
	solAssert(size <= numeric_limits<u256>::max(), "Array size does not fit u256.");
	return u256(size);
}

std::unique_ptr<ReferenceType> ArrayType::copyForLocation(DataLocation _location, bool _isPointer) const
{
	auto copy = make_unique<ArrayType>(_location);
	if (_location == DataLocation::Storage)
		copy->m_isPointer = _isPointer;
	copy->m_arrayKind = m_arrayKind;
	copy->m_baseType = copy->copyForLocationIfReference(m_baseType);
	copy->m_hasDynamicLength = m_hasDynamicLength;
	copy->m_length = m_length;
	return copy;
}

BoolResult ArraySliceType::isImplicitlyConvertibleTo(Type const& _other) const
{
	return
		(*this) == _other ||
		(
			m_arrayType.dataStoredIn(DataLocation::CallData) &&
			m_arrayType.isDynamicallySized() &&
			m_arrayType.isImplicitlyConvertibleTo(_other)
		);
}

string ArraySliceType::richIdentifier() const
{
	return m_arrayType.richIdentifier() + "_slice";
}

bool ArraySliceType::operator==(Type const& _other) const
{
	if (auto const* other = dynamic_cast<ArraySliceType const*>(&_other))
		return m_arrayType == other->m_arrayType;
	return false;
}

string ArraySliceType::toString(bool _short) const
{
	return m_arrayType.toString(_short) + " slice";
}

TypePointer ArraySliceType::mobileType() const
{
	if (
		m_arrayType.dataStoredIn(DataLocation::CallData) &&
		m_arrayType.isDynamicallySized() &&
		!m_arrayType.baseType()->isDynamicallyEncoded()
	)
		return &m_arrayType;
	else
		return this;
}


std::vector<std::tuple<std::string, TypePointer>> ArraySliceType::makeStackItems() const
{
	return {{"offset", TypeProvider::uint256()}, {"length", TypeProvider::uint256()}};
}

string ContractType::richIdentifier() const
{
	return (m_super ? "t_super" : "t_contract") + parenthesizeUserIdentifier(m_contract.name()) + to_string(m_contract.id());
}

bool ContractType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	ContractType const& other = dynamic_cast<ContractType const&>(_other);
	return other.m_contract == m_contract && other.m_super == m_super;
}

string ContractType::toString(bool) const
{
	return
		string(m_contract.isLibrary() ? "library " : "contract ") +
		string(m_super ? "super " : "") +
		m_contract.name();
}

string ContractType::canonicalName() const
{
	return *m_contract.annotation().canonicalName;
}

MemberList::MemberMap ContractType::nativeMembers(ASTNode const*) const
{
	MemberList::MemberMap members;
	solAssert(!m_super, "");
	if (!m_contract.isLibrary())
		for (auto const& it: m_contract.interfaceFunctions())
			members.emplace_back(
				it.second->declaration().name(),
				it.second->asExternallyCallableFunction(m_contract.isLibrary()),
				&it.second->declaration()
			);

	return members;
}

FunctionType const* ContractType::newExpressionType() const
{
	if (!m_constructorType)
		m_constructorType = FunctionType::newExpressionType(m_contract);
	return m_constructorType;
}

vector<tuple<VariableDeclaration const*, u256, unsigned>> ContractType::stateVariables() const
{
	vector<VariableDeclaration const*> variables;
	for (ContractDefinition const* contract: boost::adaptors::reverse(m_contract.annotation().linearizedBaseContracts))
		for (VariableDeclaration const* variable: contract->stateVariables())
			if (!(variable->isConstant() || variable->immutable()))
				variables.push_back(variable);
	TypePointers types;
	for (auto variable: variables)
		types.push_back(variable->annotation().type);
	StorageOffsets offsets;
	offsets.computeOffsets(types);

	vector<tuple<VariableDeclaration const*, u256, unsigned>> variablesAndOffsets;
	for (size_t index = 0; index < variables.size(); ++index)
		if (auto const* offset = offsets.offset(index))
			variablesAndOffsets.emplace_back(variables[index], offset->first, offset->second);
	return variablesAndOffsets;
}

vector<VariableDeclaration const*> ContractType::immutableVariables() const
{
	vector<VariableDeclaration const*> variables;
	for (ContractDefinition const* contract: boost::adaptors::reverse(m_contract.annotation().linearizedBaseContracts))
		for (VariableDeclaration const* variable: contract->stateVariables())
			if (variable->immutable())
				variables.push_back(variable);
	return variables;
}

vector<tuple<string, TypePointer>> ContractType::makeStackItems() const
{
	if (m_super)
		return {};
	else
		return {make_tuple("address", isPayable() ? TypeProvider::payableAddress() : TypeProvider::address())};
}

void StructType::clearCache() const
{
	Type::clearCache();

	m_interfaceType.reset();
	m_interfaceType_library.reset();
}

Type const* StructType::encodingType() const
{
	if (location() != DataLocation::Storage)
		return this;

	return TypeProvider::uint256();
}

BoolResult StructType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo.category() != category())
		return false;
	auto& convertTo = dynamic_cast<StructType const&>(_convertTo);
	// memory/calldata to storage can be converted, but only to a direct storage reference
	if (convertTo.location() == DataLocation::Storage && location() != DataLocation::Storage && convertTo.isPointer())
		return false;
	if (convertTo.location() == DataLocation::CallData && location() != convertTo.location())
		return false;
	return this->m_struct == convertTo.m_struct;
}

string StructType::richIdentifier() const
{
	return "t_struct" + parenthesizeUserIdentifier(m_struct.name()) + to_string(m_struct.id()) + identifierLocationSuffix();
}

bool StructType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	StructType const& other = dynamic_cast<StructType const&>(_other);
	return ReferenceType::operator==(other) && other.m_struct == m_struct;
}


unsigned StructType::calldataEncodedSize(bool) const
{
	solAssert(!isDynamicallyEncoded(), "");

	unsigned size = 0;
	for (auto const& member: members(nullptr))
	{
		solAssert(!member.type->containsNestedMapping(), "");
		// Struct members are always padded.
		size += member.type->calldataEncodedSize();
	}
	return size;
}


unsigned StructType::calldataEncodedTailSize() const
{
	solAssert(isDynamicallyEncoded(), "");

	unsigned size = 0;
	for (auto const& member: members(nullptr))
	{
		solAssert(!member.type->containsNestedMapping(), "");
		// Struct members are always padded.
		size += member.type->calldataHeadSize();
	}
	return size;
}

unsigned StructType::calldataOffsetOfMember(std::string const& _member) const
{
	unsigned offset = 0;
	for (auto const& member: members(nullptr))
	{
		solAssert(!member.type->containsNestedMapping(), "");
		if (member.name == _member)
			return offset;
		// Struct members are always padded.
		offset += member.type->calldataHeadSize();
	}
	solAssert(false, "Struct member not found.");
}

bool StructType::isDynamicallyEncoded() const
{
	if (recursive())
		return true;
	solAssert(interfaceType(false).get(), "");
	for (auto t: memoryMemberTypes())
	{
		solAssert(t, "Parameter should have external type.");
		t = t->interfaceType(false);
		if (t->isDynamicallyEncoded())
			return true;
	}
	return false;
}

u256 StructType::memoryDataSize() const
{
	u256 size;
	for (auto const& t: memoryMemberTypes())
		size += t->memoryHeadSize();
	return size;
}

bigint StructType::storageSizeUpperBound() const
{
	bigint size = 1;
	for (auto const& member: members(nullptr))
		size += member.type->storageSizeUpperBound();
	return size;
}

u256 StructType::storageSize() const
{
	return max<u256>(1, members(nullptr).storageSize());
}

bool StructType::containsNestedMapping() const
{
	if (!m_struct.annotation().containsNestedMapping.has_value())
	{
		bool hasNestedMapping = false;

		util::BreadthFirstSearch<StructDefinition const*> breadthFirstSearch{{&m_struct}};

		breadthFirstSearch.run(
			[&](StructDefinition const* _struct, auto&& _addChild)
			{
				for (auto const& member: _struct->members())
				{
					TypePointer memberType = member->annotation().type;
					solAssert(memberType, "");

					if (auto arrayType = dynamic_cast<ArrayType const*>(memberType))
						memberType = arrayType->finalBaseType(false);

					if (dynamic_cast<MappingType const*>(memberType))
					{
						hasNestedMapping = true;
						breadthFirstSearch.abort();
					}
					else if (auto structType = dynamic_cast<StructType const*>(memberType))
						_addChild(&structType->structDefinition());
				}

			});

		m_struct.annotation().containsNestedMapping = hasNestedMapping;
	}

	return m_struct.annotation().containsNestedMapping.value();
}

string StructType::toString(bool _short) const
{
	string ret = "struct " + *m_struct.annotation().canonicalName;
	if (!_short)
		ret += " " + stringForReferencePart();
	return ret;
}

MemberList::MemberMap StructType::nativeMembers(ASTNode const*) const
{
	MemberList::MemberMap members;
	for (ASTPointer<VariableDeclaration> const& variable: m_struct.members())
	{
		TypePointer type = variable->annotation().type;
		solAssert(type, "");
		solAssert(!(location() != DataLocation::Storage && type->containsNestedMapping()), "");
		members.emplace_back(
			variable->name(),
			copyForLocationIfReference(type),
			variable.get()
		);
	}
	return members;
}

TypeResult StructType::interfaceType(bool _inLibrary) const
{
	if (!_inLibrary)
	{
		if (!m_interfaceType.has_value())
		{
			if (recursive())
				m_interfaceType = TypeResult::err("Recursive type not allowed for public or external contract functions.");
			else
			{
				TypeResult result{TypePointer{}};
				for (ASTPointer<VariableDeclaration> const& member: m_struct.members())
				{
					if (!member->annotation().type)
					{
						result = TypeResult::err("Invalid type!");
						break;
					}
					auto interfaceType = member->annotation().type->interfaceType(false);
					if (!interfaceType.get())
					{
						solAssert(!interfaceType.message().empty(), "Expected detailed error message!");
						result = interfaceType;
						break;
					}
				}
				if (result.message().empty())
					m_interfaceType = TypeProvider::withLocation(this, DataLocation::Memory, true);
				else
					m_interfaceType = result;
			}
		}
		return *m_interfaceType;
	}
	else if (m_interfaceType_library.has_value())
		return *m_interfaceType_library;

	TypeResult result{TypePointer{}};

	if (recursive() && !(_inLibrary && location() == DataLocation::Storage))
		return TypeResult::err(
			"Recursive structs can only be passed as storage pointers to libraries, "
			"not as memory objects to contract functions."
		);

	util::BreadthFirstSearch<StructDefinition const*> breadthFirstSearch{{&m_struct}};
	breadthFirstSearch.run(
		[&](StructDefinition const* _struct, auto&& _addChild)
		{
			// Check that all members have interface types.
			// Return an error if at least one struct member does not have a type.
			// This might happen, for example, if the type of the member does not exist.
			for (ASTPointer<VariableDeclaration> const& variable: _struct->members())
			{
				// If the struct member does not have a type return false.
				// A TypeError is expected in this case.
				if (!variable->annotation().type)
				{
					result = TypeResult::err("Invalid type!");
					breadthFirstSearch.abort();
					return;
				}

				Type const* memberType = variable->annotation().type;

				while (
					memberType->category() == Type::Category::Array ||
					memberType->category() == Type::Category::Mapping
				)
				{
					if (auto arrayType = dynamic_cast<ArrayType const*>(memberType))
						memberType = arrayType->finalBaseType(false);
					else if (auto mappingType = dynamic_cast<MappingType const*>(memberType))
						memberType = mappingType->valueType();
				}

				if (StructType const* innerStruct = dynamic_cast<StructType const*>(memberType))
					_addChild(&innerStruct->structDefinition());
				else
				{
					auto iType = memberType->interfaceType(_inLibrary);
					if (!iType.get())
					{
						solAssert(!iType.message().empty(), "Expected detailed error message!");
						result = iType;
						breadthFirstSearch.abort();
						return;
					}
				}
			}
		}
	);

	if (!result.message().empty())
		return result;

	if (location() == DataLocation::Storage)
		m_interfaceType_library = this;
	else
		m_interfaceType_library = TypeProvider::withLocation(this, DataLocation::Memory, true);
	return *m_interfaceType_library;
}

BoolResult StructType::validForLocation(DataLocation _loc) const
{
	for (auto const& member: m_struct.members())
		if (auto referenceType = dynamic_cast<ReferenceType const*>(member->annotation().type))
		{
			BoolResult result = referenceType->validForLocation(_loc);
			if (!result)
				return result;
		}

	if (
		_loc == DataLocation::Storage &&
		storageSizeUpperBound() >= bigint(1) << 256
	)
		return BoolResult::err("Type too large for storage.");

	return true;
}

bool StructType::recursive() const
{
	solAssert(m_struct.annotation().recursive.has_value(), "Called StructType::recursive() before DeclarationTypeChecker.");
	return *m_struct.annotation().recursive;
}

std::unique_ptr<ReferenceType> StructType::copyForLocation(DataLocation _location, bool _isPointer) const
{
	auto copy = make_unique<StructType>(m_struct, _location);
	if (_location == DataLocation::Storage)
		copy->m_isPointer = _isPointer;
	return copy;
}

string StructType::signatureInExternalFunction(bool _structsByName) const
{
	if (_structsByName)
		return canonicalName();
	else
	{
		TypePointers memberTypes = memoryMemberTypes();
		auto memberTypeStrings = memberTypes | boost::adaptors::transformed([&](TypePointer _t) -> string
		{
			solAssert(_t, "Parameter should have external type.");
			auto t = _t->interfaceType(_structsByName);
			solAssert(t.get(), "");
			return t.get()->signatureInExternalFunction(_structsByName);
		});
		return "(" + boost::algorithm::join(memberTypeStrings, ",") + ")";
	}
}

string StructType::canonicalName() const
{
	return *m_struct.annotation().canonicalName;
}

FunctionTypePointer StructType::constructorType() const
{
	TypePointers paramTypes;
	strings paramNames;
	solAssert(!containsNestedMapping(), "");
	for (auto const& member: members(nullptr))
	{
		paramNames.push_back(member.name);
		paramTypes.push_back(TypeProvider::withLocationIfReference(DataLocation::Memory, member.type));
	}
	return TypeProvider::function(
		paramTypes,
		TypePointers{TypeProvider::withLocation(this, DataLocation::Memory, false)},
		paramNames,
		strings(1, ""),
		FunctionType::Kind::Internal
	);
}

pair<u256, unsigned> const& StructType::storageOffsetsOfMember(string const& _name) const
{
	auto const* offsets = members(nullptr).memberStorageOffset(_name);
	solAssert(offsets, "Storage offset of non-existing member requested.");
	return *offsets;
}

u256 StructType::memoryOffsetOfMember(string const& _name) const
{
	u256 offset;
	for (auto const& member: members(nullptr))
		if (member.name == _name)
			return offset;
		else
			offset += member.type->memoryHeadSize();
	solAssert(false, "Member not found in struct.");
	return 0;
}

TypePointers StructType::memoryMemberTypes() const
{
	solAssert(!containsNestedMapping(), "");
	TypePointers types;
	for (ASTPointer<VariableDeclaration> const& variable: m_struct.members())
		types.push_back(TypeProvider::withLocationIfReference(DataLocation::Memory, variable->annotation().type));

	return types;
}

vector<tuple<string, TypePointer>> StructType::makeStackItems() const
{
	switch (m_location)
	{
		case DataLocation::CallData:
			return {std::make_tuple("offset", TypeProvider::uint256())};
		case DataLocation::Memory:
			return {std::make_tuple("mpos", TypeProvider::uint256())};
		case DataLocation::Storage:
			return {std::make_tuple("slot", TypeProvider::uint256())};
	}
	solAssert(false, "");
}

vector<Type const*> StructType::decomposition() const
{
	vector<Type const*> res;
	for (MemberList::Member const& member: members(nullptr))
		res.push_back(member.type);
	return res;
}

TypePointer EnumType::encodingType() const
{
	solAssert(numberOfMembers() <= 256, "");
	return TypeProvider::uint(8);
}

TypeResult EnumType::unaryOperatorResult(Token _operator) const
{
	return _operator == Token::Delete ? TypeProvider::emptyTuple() : nullptr;
}

string EnumType::richIdentifier() const
{
	return "t_enum" + parenthesizeUserIdentifier(m_enum.name()) + to_string(m_enum.id());
}

bool EnumType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	EnumType const& other = dynamic_cast<EnumType const&>(_other);
	return other.m_enum == m_enum;
}

unsigned EnumType::storageBytes() const
{
	solAssert(numberOfMembers() <= 256, "");
	return 1;
}

string EnumType::toString(bool) const
{
	return string("enum ") + *m_enum.annotation().canonicalName;
}

string EnumType::canonicalName() const
{
	return *m_enum.annotation().canonicalName;
}

size_t EnumType::numberOfMembers() const
{
	return m_enum.members().size();
}

BoolResult EnumType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo == *this)
		return true;
	else if (auto integerType = dynamic_cast<IntegerType const*>(&_convertTo))
		return !integerType->isSigned();
	return false;
}

unsigned EnumType::memberValue(ASTString const& _member) const
{
	unsigned index = 0;
	for (ASTPointer<EnumValue> const& decl: m_enum.members())
	{
		if (decl->name() == _member)
			return index;
		++index;
	}
	solAssert(false, "Requested unknown enum value " + _member);
}

BoolResult TupleType::isImplicitlyConvertibleTo(Type const& _other) const
{
	if (auto tupleType = dynamic_cast<TupleType const*>(&_other))
	{
		TypePointers const& targets = tupleType->components();
		if (targets.empty())
			return components().empty();
		if (components().size() != targets.size())
			return false;
		for (size_t i = 0; i < targets.size(); ++i)
			if (!components()[i] && targets[i])
				return false;
			else if (components()[i] && targets[i] && !components()[i]->isImplicitlyConvertibleTo(*targets[i]))
				return false;
		return true;
	}
	else
		return false;
}

string TupleType::richIdentifier() const
{
	return "t_tuple" + identifierList(components());
}

bool TupleType::operator==(Type const& _other) const
{
	if (auto tupleType = dynamic_cast<TupleType const*>(&_other))
		return components() == tupleType->components();
	else
		return false;
}

string TupleType::toString(bool _short) const
{
	if (components().empty())
		return "tuple()";
	string str = "tuple(";
	for (auto const& t: components())
		str += (t ? t->toString(_short) : "") + ",";
	str.pop_back();
	return str + ")";
}

u256 TupleType::storageSize() const
{
	solAssert(false, "Storage size of non-storable tuple type requested.");
}

vector<tuple<string, TypePointer>> TupleType::makeStackItems() const
{
	vector<tuple<string, TypePointer>> slots;
	unsigned i = 1;
	for (auto const& t: components())
	{
		if (t)
			slots.emplace_back("component_" + std::to_string(i), t);
		++i;
	}
	return slots;
}

TypePointer TupleType::mobileType() const
{
	TypePointers mobiles;
	for (auto const& c: components())
	{
		if (c)
		{
			auto mt = c->mobileType();
			if (!mt)
				return nullptr;
			mobiles.push_back(mt);
		}
		else
			mobiles.push_back(nullptr);
	}
	return TypeProvider::tuple(move(mobiles));
}

TypePointer TupleType::closestTemporaryType(Type const* _targetType) const
{
	solAssert(!!_targetType, "");
	TypePointers const& targetComponents = dynamic_cast<TupleType const&>(*_targetType).components();
	solAssert(components().size() == targetComponents.size(), "");
	TypePointers tempComponents(targetComponents.size());
	for (size_t i = 0; i < targetComponents.size(); ++i)
	{
		if (components()[i] && targetComponents[i])
		{
			tempComponents[i] = components()[i]->closestTemporaryType(targetComponents[i]);
			solAssert(tempComponents[i], "");
		}
	}
	return TypeProvider::tuple(move(tempComponents));
}

FunctionType::FunctionType(FunctionDefinition const& _function, Kind _kind):
	m_kind(_kind),
	m_stateMutability(_function.stateMutability()),
	m_declaration(&_function)
{
	solAssert(
		_kind == Kind::Internal || _kind == Kind::External || _kind == Kind::Declaration,
		"Only internal or external function types or function declaration types can be created from function definitions."
	);
	if (_kind == Kind::Internal && m_stateMutability == StateMutability::Payable)
		m_stateMutability = StateMutability::NonPayable;

	for (ASTPointer<VariableDeclaration> const& var: _function.parameters())
	{
		m_parameterNames.push_back(var->name());
		m_parameterTypes.push_back(var->annotation().type);
	}
	for (ASTPointer<VariableDeclaration> const& var: _function.returnParameters())
	{
		m_returnParameterNames.push_back(var->name());
		m_returnParameterTypes.push_back(var->annotation().type);
	}

	solAssert(
		m_parameterNames.size() == m_parameterTypes.size(),
		"Parameter names list must match parameter types list!"
	);

	solAssert(
		m_returnParameterNames.size() == m_returnParameterTypes.size(),
		"Return parameter names list must match return parameter types list!"
	);
}

FunctionType::FunctionType(VariableDeclaration const& _varDecl):
	m_kind(Kind::External),
	m_stateMutability(StateMutability::View),
	m_declaration(&_varDecl)
{
	auto returnType = _varDecl.annotation().type;

	while (true)
	{
		if (auto mappingType = dynamic_cast<MappingType const*>(returnType))
		{
			m_parameterTypes.push_back(mappingType->keyType());
			m_parameterNames.emplace_back("");
			returnType = mappingType->valueType();
		}
		else if (auto arrayType = dynamic_cast<ArrayType const*>(returnType))
		{
			if (arrayType->isByteArray())
				// Return byte arrays as whole.
				break;
			returnType = arrayType->baseType();
			m_parameterNames.emplace_back("");
			m_parameterTypes.push_back(TypeProvider::uint256());
		}
		else
			break;
	}

	if (auto structType = dynamic_cast<StructType const*>(returnType))
	{
		for (auto const& member: structType->members(nullptr))
		{
			solAssert(member.type, "");
			if (member.type->category() != Category::Mapping)
			{
				if (auto arrayType = dynamic_cast<ArrayType const*>(member.type))
					if (!arrayType->isByteArray())
						continue;
				m_returnParameterTypes.push_back(TypeProvider::withLocationIfReference(
					DataLocation::Memory,
					member.type
				));
				m_returnParameterNames.push_back(member.name);
			}
		}
	}
	else
	{
		m_returnParameterTypes.push_back(TypeProvider::withLocationIfReference(
			DataLocation::Memory,
			returnType
		));
		m_returnParameterNames.emplace_back("");
	}

	solAssert(
			m_parameterNames.size() == m_parameterTypes.size(),
			"Parameter names list must match parameter types list!"
			);
	solAssert(
			m_returnParameterNames.size() == m_returnParameterTypes.size(),
			"Return parameter names list must match return parameter types list!"
			);
}

FunctionType::FunctionType(EventDefinition const& _event):
	m_kind(Kind::Event),
	m_stateMutability(StateMutability::NonPayable),
	m_declaration(&_event)
{
	for (ASTPointer<VariableDeclaration> const& var: _event.parameters())
	{
		m_parameterNames.push_back(var->name());
		m_parameterTypes.push_back(var->annotation().type);
	}

	solAssert(
			m_parameterNames.size() == m_parameterTypes.size(),
			"Parameter names list must match parameter types list!"
			);
	solAssert(
			m_returnParameterNames.size() == m_returnParameterTypes.size(),
			"Return parameter names list must match return parameter types list!"
			);
}

FunctionType::FunctionType(FunctionTypeName const& _typeName):
	m_parameterNames(_typeName.parameterTypes().size(), ""),
	m_returnParameterNames(_typeName.returnParameterTypes().size(), ""),
	m_kind(_typeName.visibility() == Visibility::External ? Kind::External : Kind::Internal),
	m_stateMutability(_typeName.stateMutability())
{
	if (_typeName.isPayable())
		solAssert(m_kind == Kind::External, "Internal payable function type used.");
	for (auto const& t: _typeName.parameterTypes())
	{
		solAssert(t->annotation().type, "Type not set for parameter.");
		m_parameterTypes.push_back(t->annotation().type);
	}
	for (auto const& t: _typeName.returnParameterTypes())
	{
		solAssert(t->annotation().type, "Type not set for return parameter.");
		m_returnParameterTypes.push_back(t->annotation().type);
	}

	solAssert(
			m_parameterNames.size() == m_parameterTypes.size(),
			"Parameter names list must match parameter types list!"
			);
	solAssert(
			m_returnParameterNames.size() == m_returnParameterTypes.size(),
			"Return parameter names list must match return parameter types list!"
			);
}

FunctionTypePointer FunctionType::newExpressionType(ContractDefinition const& _contract)
{
	FunctionDefinition const* constructor = _contract.constructor();
	TypePointers parameters;
	strings parameterNames;
	StateMutability stateMutability = StateMutability::NonPayable;

	solAssert(!_contract.isInterface(), "");

	if (constructor)
	{
		for (ASTPointer<VariableDeclaration> const& var: constructor->parameters())
		{
			parameterNames.push_back(var->name());
			parameters.push_back(var->annotation().type);
		}
		if (constructor->isPayable())
			stateMutability = StateMutability::Payable;
	}

	return TypeProvider::function(
		parameters,
		TypePointers{TypeProvider::contract(_contract)},
		parameterNames,
		strings{""},
		Kind::Creation,
		false,
		stateMutability
	);
}

vector<string> FunctionType::parameterNames() const
{
	if (!bound())
		return m_parameterNames;
	return vector<string>(m_parameterNames.cbegin() + 1, m_parameterNames.cend());
}

TypePointers FunctionType::returnParameterTypesWithoutDynamicTypes() const
{
	TypePointers returnParameterTypes = m_returnParameterTypes;

	if (
		m_kind == Kind::External ||
		m_kind == Kind::DelegateCall ||
		m_kind == Kind::BareCall ||
		m_kind == Kind::BareCallCode ||
		m_kind == Kind::BareDelegateCall ||
		m_kind == Kind::BareStaticCall
	)
		for (auto& param: returnParameterTypes)
		{
			solAssert(param->decodingType(), "");
			if (param->decodingType()->isDynamicallyEncoded())
				param = TypeProvider::inaccessibleDynamic();
		}

	return returnParameterTypes;
}

TypePointers FunctionType::parameterTypes() const
{
	if (!bound())
		return m_parameterTypes;
	return TypePointers(m_parameterTypes.cbegin() + 1, m_parameterTypes.cend());
}

TypePointers const& FunctionType::parameterTypesIncludingSelf() const
{
	return m_parameterTypes;
}

string FunctionType::richIdentifier() const
{
	string id = "t_function_";
	switch (m_kind)
	{
	case Kind::Declaration: id += "declaration"; break;
	case Kind::Internal: id += "internal"; break;
	case Kind::External: id += "external"; break;
	case Kind::DelegateCall: id += "delegatecall"; break;
	case Kind::BareCall: id += "barecall"; break;
	case Kind::BareCallCode: id += "barecallcode"; break;
	case Kind::BareDelegateCall: id += "baredelegatecall"; break;
	case Kind::BareStaticCall: id += "barestaticcall"; break;
	case Kind::Creation: id += "creation"; break;
	case Kind::Send: id += "send"; break;
	case Kind::Transfer: id += "transfer"; break;
	case Kind::KECCAK256: id += "keccak256"; break;
	case Kind::Selfdestruct: id += "selfdestruct"; break;
	case Kind::Revert: id += "revert"; break;
	case Kind::ECRecover: id += "ecrecover"; break;
	case Kind::SHA256: id += "sha256"; break;
	case Kind::RIPEMD160: id += "ripemd160"; break;
	case Kind::GasLeft: id += "gasleft"; break;
	case Kind::Event: id += "event"; break;
	case Kind::SetGas: id += "setgas"; break;
	case Kind::SetValue: id += "setvalue"; break;
	case Kind::BlockHash: id += "blockhash"; break;
	case Kind::AddMod: id += "addmod"; break;
	case Kind::MulMod: id += "mulmod"; break;
	case Kind::ArrayPush: id += "arraypush"; break;
	case Kind::ArrayPop: id += "arraypop"; break;
	case Kind::ByteArrayPush: id += "bytearraypush"; break;
	case Kind::ObjectCreation: id += "objectcreation"; break;
	case Kind::Assert: id += "assert"; break;
	case Kind::Require: id += "require"; break;
	case Kind::ABIEncode: id += "abiencode"; break;
	case Kind::ABIEncodePacked: id += "abiencodepacked"; break;
	case Kind::ABIEncodeWithSelector: id += "abiencodewithselector"; break;
	case Kind::ABIEncodeWithSignature: id += "abiencodewithsignature"; break;
	case Kind::ABIDecode: id += "abidecode"; break;
	case Kind::MetaType: id += "metatype"; break;
	// Solidity++:
	case Kind::BLAKE2B: id += "blake2b"; break;
	case Kind::PrevHash: id += "prevhash"; break;
	case Kind::Height: id += "height"; break;
	case Kind::AccountHeight: id += "accountheight"; break;
	case Kind::FromHash: id += "fromhash"; break;
	case Kind::Random64: id += "random64"; break;
	case Kind::NextRandom: id += "nextrandom"; break;
	case Kind::Balance: id += "balance"; break;

	}
	id += "_" + stateMutabilityToString(m_stateMutability);
	id += identifierList(m_parameterTypes) + "returns" + identifierList(m_returnParameterTypes);
	if (m_gasSet)
		id += "gas";
	if (m_valueSet)
		id += "value";
	if (m_tokenSet)  // Solidity++
	    id += "tti";
	if (m_saltSet)
		id += "salt";
	if (bound())
		id += "bound_to" + identifierList(selfType());
	return id;
}

bool FunctionType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	FunctionType const& other = dynamic_cast<FunctionType const&>(_other);
	if (!equalExcludingStateMutability(other))
		return false;
	if (m_stateMutability != other.stateMutability())
		return false;
	return true;
}

BoolResult FunctionType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo.category() == category())
	{
		auto const& convertToType = dynamic_cast<FunctionType const&>(_convertTo);
		return (m_kind == FunctionType::Kind::Declaration) == (convertToType.kind() == FunctionType::Kind::Declaration);
	}
	return false;
}

BoolResult FunctionType::isImplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (_convertTo.category() != category())
		return false;

	FunctionType const& convertTo = dynamic_cast<FunctionType const&>(_convertTo);

	// These two checks are duplicated in equalExcludingStateMutability, but are added here for error reporting.
	if (convertTo.bound() != bound())
		return BoolResult::err("Bound functions can not be converted to non-bound functions.");

	if (convertTo.kind() != kind())
		return BoolResult::err("Special functions can not be converted to function types.");

	if (!equalExcludingStateMutability(convertTo))
		return false;

	// non-payable should not be convertible to payable
	if (m_stateMutability != StateMutability::Payable && convertTo.stateMutability() == StateMutability::Payable)
		return false;

	// payable should be convertible to non-payable, because you are free to pay 0 ether
	if (m_stateMutability == StateMutability::Payable && convertTo.stateMutability() == StateMutability::NonPayable)
		return true;

	// e.g. pure should be convertible to view, but not the other way around.
	if (m_stateMutability > convertTo.stateMutability())
		return false;

	return true;
}

TypeResult FunctionType::unaryOperatorResult(Token _operator) const
{
	if (_operator == Token::Delete)
		return TypeResult(TypeProvider::emptyTuple());
	return nullptr;
}

TypeResult FunctionType::binaryOperatorResult(Token _operator, Type const* _other) const
{
	if (_other->category() != category() || !(_operator == Token::Equal || _operator == Token::NotEqual))
		return nullptr;
	FunctionType const& other = dynamic_cast<FunctionType const&>(*_other);
	if (kind() == Kind::Internal && other.kind() == Kind::Internal && sizeOnStack() == 1 && other.sizeOnStack() == 1)
		return commonType(this, _other);
	return nullptr;
}

string FunctionType::canonicalName() const
{
	solAssert(m_kind == Kind::External, "");
	return "function";
}

string FunctionType::toString(bool _short) const
{
	string name = "function ";
	if (m_kind == Kind::Declaration)
	{
		auto const* functionDefinition = dynamic_cast<FunctionDefinition const*>(m_declaration);
		solAssert(functionDefinition, "");
		if (auto const* contract = dynamic_cast<ContractDefinition const*>(functionDefinition->scope()))
			name += *contract->annotation().canonicalName + ".";
		name += functionDefinition->name();
	}
	name += '(';
	for (auto it = m_parameterTypes.begin(); it != m_parameterTypes.end(); ++it)
		name += (*it)->toString(_short) + (it + 1 == m_parameterTypes.end() ? "" : ",");
	name += ")";
	if (m_stateMutability != StateMutability::NonPayable)
		name += " " + stateMutabilityToString(m_stateMutability);
	if (m_kind == Kind::External)
		name += " external";
	if (!m_returnParameterTypes.empty())
	{
		name += " returns (";
		for (auto it = m_returnParameterTypes.begin(); it != m_returnParameterTypes.end(); ++it)
			name += (*it)->toString(_short) + (it + 1 == m_returnParameterTypes.end() ? "" : ",");
		name += ")";
	}
	return name;
}

unsigned FunctionType::calldataEncodedSize(bool _padded) const
{
	unsigned size = storageBytes();
	if (_padded)
		size = ((size + 31) / 32) * 32;
	return size;
}

u256 FunctionType::storageSize() const
{
	if (m_kind == Kind::External || m_kind == Kind::Internal)
		return 1;
	else
		solAssert(false, "Storage size of non-storable function type requested.");
}

bool FunctionType::leftAligned() const
{
	if (m_kind == Kind::External)
		return true;
	else
		solAssert(false, "Alignment property of non-exportable function type requested.");
}

unsigned FunctionType::storageBytes() const
{
	if (m_kind == Kind::External)
		return 21 + 4;  // Solidity++: 168-bit address
	else if (m_kind == Kind::Internal)
		return 8; // it should really not be possible to create larger programs
	else
		solAssert(false, "Storage size of non-storable function type requested.");
}

bool FunctionType::nameable() const
{
	return
		(m_kind == Kind::Internal || m_kind == Kind::External) &&
		!m_bound &&
		!m_arbitraryParameters &&
		!m_gasSet &&
		!m_valueSet &&
		!m_tokenSet &&
		!m_saltSet;
}

vector<tuple<string, TypePointer>> FunctionType::makeStackItems() const
{
	vector<tuple<string, TypePointer>> slots;
	Kind kind = m_kind;
	if (m_kind == Kind::SetGas || m_kind == Kind::SetValue)
	{
		solAssert(m_returnParameterTypes.size() == 1, "");
		kind = dynamic_cast<FunctionType const&>(*m_returnParameterTypes.front()).m_kind;
	}

	switch (kind)
	{
	case Kind::External:
	case Kind::DelegateCall:
		slots = {
			make_tuple("address", TypeProvider::address()),
			make_tuple("functionSelector", TypeProvider::uint(32))
		};
		break;
	case Kind::BareCall:
	case Kind::BareCallCode:
	case Kind::BareDelegateCall:
	case Kind::BareStaticCall:
	case Kind::Transfer:
	case Kind::Send:
		slots = {make_tuple("address", TypeProvider::address())};
		break;
	case Kind::Internal:
		slots = {make_tuple("functionIdentifier", TypeProvider::uint256())};
		break;
	case Kind::ArrayPush:
	case Kind::ArrayPop:
	case Kind::ByteArrayPush:
		slots = {make_tuple("slot", TypeProvider::uint256())};
		break;
	default:
		break;
	}

	if (m_gasSet)
		slots.emplace_back("gas", TypeProvider::uint256());
	if (m_valueSet)
		slots.emplace_back("value", TypeProvider::uint256());
	if (m_tokenSet)
	    slots.emplace_back("tti", TypeProvider::viteTokenId());  // Solidity++
	if (m_saltSet)
		slots.emplace_back("salt", TypeProvider::fixedBytes(32));
	if (bound())
		slots.emplace_back("self", m_parameterTypes.front());
	return slots;
}

FunctionTypePointer FunctionType::interfaceFunctionType() const
{
	// Note that m_declaration might also be a state variable!
	solAssert(m_declaration, "Declaration needed to determine interface function type.");
	bool isLibraryFunction = false;
	if (kind() != Kind::Event)
		if (auto const* contract = dynamic_cast<ContractDefinition const*>(m_declaration->scope()))
			isLibraryFunction = contract->isLibrary();

	util::Result<TypePointers> paramTypes =
		transformParametersToExternal(m_parameterTypes, isLibraryFunction);

	if (!paramTypes.message().empty())
		return FunctionTypePointer();

	util::Result<TypePointers> retParamTypes =
		transformParametersToExternal(m_returnParameterTypes, isLibraryFunction);

	if (!retParamTypes.message().empty())
		return FunctionTypePointer();

	auto variable = dynamic_cast<VariableDeclaration const*>(m_declaration);
	if (variable && retParamTypes.get().empty())
		return FunctionTypePointer();

	return TypeProvider::function(
		paramTypes,
		retParamTypes,
		m_parameterNames,
		m_returnParameterNames,
		m_kind,
		m_arbitraryParameters,
		m_stateMutability,
		m_declaration
	);
}

MemberList::MemberMap FunctionType::nativeMembers(ASTNode const* _scope) const
{
	switch (m_kind)
	{
	case Kind::Declaration:
		if (declaration().isPartOfExternalInterface())
			return {{"selector", TypeProvider::fixedBytes(4)}};
		else
			return MemberList::MemberMap();
	case Kind::Internal:
		if (
			auto const* functionDefinition = dynamic_cast<FunctionDefinition const*>(m_declaration);
			functionDefinition &&
			_scope &&
			functionDefinition->annotation().contract &&
			_scope != functionDefinition->annotation().contract &&
			functionDefinition->isPartOfExternalInterface()
		)
		{
			auto const* contractScope = dynamic_cast<ContractDefinition const*>(_scope);
			solAssert(contractScope && contractScope->derivesFrom(*functionDefinition->annotation().contract), "");
			return {{"selector", TypeProvider::fixedBytes(4)}};
		}
		else
			return MemberList::MemberMap();
	case Kind::External:
	case Kind::Creation:
	case Kind::BareCall:
	case Kind::BareCallCode:
	case Kind::BareDelegateCall:
	case Kind::BareStaticCall:
	{
		MemberList::MemberMap members;
		if (m_kind == Kind::External)
		{
			members.emplace_back("selector", TypeProvider::fixedBytes(4));
			members.emplace_back("address", TypeProvider::address());
		}
		if (m_kind != Kind::BareDelegateCall)
		{
			if (isPayable())
				members.emplace_back(
					"value",
					TypeProvider::function(
						parseElementaryTypeVector({"uint"}),
						TypePointers{copyAndSetCallOptions(false, true, false)},
						strings(1, ""),
						strings(1, ""),
						Kind::SetValue,
						false,
						StateMutability::Pure,
						nullptr,
						m_gasSet,
						m_valueSet,
						m_tokenSet,
						m_saltSet
					)
				);
		}
		if (m_kind != Kind::Creation)
			members.emplace_back(
				"gas",
				TypeProvider::function(
					parseElementaryTypeVector({"uint"}),
					TypePointers{copyAndSetCallOptions(true, false, false)},
					strings(1, ""),
					strings(1, ""),
					Kind::SetGas,
					false,
					StateMutability::Pure,
					nullptr,
					m_gasSet,
					m_valueSet,
					m_tokenSet,
					m_saltSet
				)
			);
		return members;
	}
	case Kind::DelegateCall:
	{
		auto const* functionDefinition = dynamic_cast<FunctionDefinition const*>(m_declaration);
		solAssert(functionDefinition, "");
		solAssert(functionDefinition->visibility() != Visibility::Private, "");
		if (functionDefinition->visibility() != Visibility::Internal)
		{
			auto const* contract = dynamic_cast<ContractDefinition const*>(m_declaration->scope());
			solAssert(contract, "");
			solAssert(contract->isLibrary(), "");
			return {{"selector", TypeProvider::fixedBytes(4)}};
		}
		return {};
	}
	default:
		return MemberList::MemberMap();
	}
}

TypePointer FunctionType::encodingType() const
{
	if (m_gasSet || m_valueSet || m_tokenSet)
		return nullptr;
	// Only external functions can be encoded, internal functions cannot leave code boundaries.
	if (m_kind == Kind::External)
		return this;
	else
		return nullptr;
}

TypeResult FunctionType::interfaceType(bool /*_inLibrary*/) const
{
	if (m_kind == Kind::External)
		return this;
	else
		return TypeResult::err("Internal type is not allowed for public or external functions.");
}

TypePointer FunctionType::mobileType() const
{
	if (m_valueSet || m_tokenSet || m_gasSet || m_saltSet || m_bound)
		return nullptr;

	// return function without parameter names
	return TypeProvider::function(
		m_parameterTypes,
		m_returnParameterTypes,
		strings(m_parameterTypes.size()),
		strings(m_returnParameterNames.size()),
		m_kind,
		m_arbitraryParameters,
		m_stateMutability,
		m_declaration,
		m_gasSet,
		m_valueSet,
		m_tokenSet,
		m_bound,
		m_saltSet
	);
}

bool FunctionType::canTakeArguments(
	FuncCallArguments const& _arguments,
	Type const* _selfType
) const
{
	solAssert(!bound() || _selfType, "");
	if (bound() && !_selfType->isImplicitlyConvertibleTo(*selfType()))
		return false;
	TypePointers paramTypes = parameterTypes();
	std::vector<std::string> const paramNames = parameterNames();

	if (takesArbitraryParameters())
		return true;
	else if (_arguments.numArguments() != paramTypes.size())
		return false;
	else if (!_arguments.hasNamedArguments())
		return equal(
			_arguments.types.cbegin(),
			_arguments.types.cend(),
			paramTypes.cbegin(),
			[](Type const* argumentType, Type const* parameterType)
			{
				return argumentType->isImplicitlyConvertibleTo(*parameterType);
			}
		);
	else if (paramNames.size() != _arguments.numNames())
		return false;
	else
	{
		solAssert(_arguments.numArguments() == _arguments.numNames(), "Expected equal sized type & name vectors");

		size_t matchedNames = 0;

		for (size_t a = 0; a < _arguments.names.size(); a++)
			for (size_t p = 0; p < paramNames.size(); p++)
				if (*_arguments.names[a] == paramNames[p])
				{
					matchedNames++;
					if (!_arguments.types[a]->isImplicitlyConvertibleTo(*paramTypes[p]))
						return false;
				}

		if (matchedNames == _arguments.numNames())
			return true;

		return false;
	}
}

bool FunctionType::hasEqualParameterTypes(FunctionType const& _other) const
{
	if (m_parameterTypes.size() != _other.m_parameterTypes.size())
		return false;
	return equal(
		m_parameterTypes.cbegin(),
		m_parameterTypes.cend(),
		_other.m_parameterTypes.cbegin(),
		[](Type const* _a, Type const* _b) -> bool { return *_a == *_b; }
	);
}

bool FunctionType::hasEqualReturnTypes(FunctionType const& _other) const
{
	if (m_returnParameterTypes.size() != _other.m_returnParameterTypes.size())
		return false;
	return equal(
		m_returnParameterTypes.cbegin(),
		m_returnParameterTypes.cend(),
		_other.m_returnParameterTypes.cbegin(),
		[](Type const* _a, Type const* _b) -> bool { return *_a == *_b; }
	);
}

bool FunctionType::equalExcludingStateMutability(FunctionType const& _other) const
{
	if (m_kind != _other.m_kind)
		return false;

	if (!hasEqualParameterTypes(_other) || !hasEqualReturnTypes(_other))
		return false;

	//@todo this is ugly, but cannot be prevented right now
	if (m_gasSet != _other.m_gasSet || m_valueSet != _other.m_valueSet || m_tokenSet != _other.m_tokenSet || m_saltSet != _other.m_saltSet)
		return false;

	if (bound() != _other.bound())
		return false;

	solAssert(!bound() || *selfType() == *_other.selfType(), "");

	return true;
}

bool FunctionType::isBareCall() const
{
	switch (m_kind)
	{
	case Kind::BareCall:
	case Kind::BareCallCode:
	case Kind::BareDelegateCall:
	case Kind::BareStaticCall:
	case Kind::ECRecover:
	case Kind::SHA256:
	case Kind::RIPEMD160:
		return true;
	default:
		return false;
	}
}

string FunctionType::externalSignature() const
{
	solAssert(m_declaration != nullptr, "External signature of function needs declaration");
	solAssert(!m_declaration->name().empty(), "Fallback function has no signature.");
	switch (kind())
	{
	case Kind::Internal:
	case Kind::External:
	case Kind::DelegateCall:
	case Kind::Event:
	case Kind::Declaration:
		break;
	default:
		solAssert(false, "Invalid function type for requesting external signature.");
	}

	// "inLibrary" is only relevant if this is not an event.
	bool inLibrary = false;
	if (kind() != Kind::Event)
		if (auto const* contract = dynamic_cast<ContractDefinition const*>(m_declaration->scope()))
			inLibrary = contract->isLibrary();

	auto extParams = transformParametersToExternal(m_parameterTypes, inLibrary);

	solAssert(extParams.message().empty(), extParams.message());

	auto typeStrings = extParams.get() | boost::adaptors::transformed([&](TypePointer _t) -> string
	{
		string typeName = _t->signatureInExternalFunction(inLibrary);

		if (inLibrary && _t->dataStoredIn(DataLocation::Storage))
			typeName += " storage";
		return typeName;
	});
	return m_declaration->name() + "(" + boost::algorithm::join(typeStrings, ",") + ")";
}

u256 FunctionType::externalIdentifier() const
{
	return util::selectorFromSignature32(externalSignature());
}

string FunctionType::externalIdentifierHex() const
{
	// Solidity++: Use blake2b instead of Keccak256
	return util::FixedHash<4>(util::blake2b(externalSignature())).hex();
}

bool FunctionType::isPure() const
{
	// TODO: replace this with m_stateMutability == StateMutability::Pure once
	//       the callgraph analyzer is in place
	return
		m_kind == Kind::KECCAK256 ||
		m_kind == Kind::ECRecover ||
		m_kind == Kind::SHA256 ||
		m_kind == Kind::RIPEMD160 ||
		m_kind == Kind::AddMod ||
		m_kind == Kind::MulMod ||
		m_kind == Kind::ObjectCreation ||
		m_kind == Kind::ABIEncode ||
		m_kind == Kind::ABIEncodePacked ||
		m_kind == Kind::ABIEncodeWithSelector ||
		m_kind == Kind::ABIEncodeWithSignature ||
		m_kind == Kind::ABIDecode ||
		m_kind == Kind::MetaType;
}

TypePointers FunctionType::parseElementaryTypeVector(strings const& _types)
{
	TypePointers pointers;
	pointers.reserve(_types.size());
	for (string const& type: _types)
		pointers.push_back(TypeProvider::fromElementaryTypeName(type));
	return pointers;
}

TypePointer FunctionType::copyAndSetCallOptions(bool _setGas, bool _setValue, bool _setSalt, bool _setToken) const
{
	solAssert(m_kind != Kind::Declaration, "");
	return TypeProvider::function(
		m_parameterTypes,
		m_returnParameterTypes,
		m_parameterNames,
		m_returnParameterNames,
		m_kind,
		m_arbitraryParameters,
		m_stateMutability,
		m_declaration,
		m_gasSet || _setGas,
		m_valueSet || _setValue,
		m_tokenSet || _setToken,
		m_saltSet || _setSalt,
		m_bound
	);
}

FunctionTypePointer FunctionType::asBoundFunction() const
{
	solAssert(!m_parameterTypes.empty(), "");
	FunctionDefinition const* fun = dynamic_cast<FunctionDefinition const*>(m_declaration);
	solAssert(fun && fun->libraryFunction(), "");
	solAssert(!m_gasSet, "");
	solAssert(!m_valueSet, "");
	solAssert(!m_tokenSet, "");
	solAssert(!m_saltSet, "");
	return TypeProvider::function(
		m_parameterTypes,
		m_returnParameterTypes,
		m_parameterNames,
		m_returnParameterNames,
		m_kind,
		m_arbitraryParameters,
		m_stateMutability,
		m_declaration,
		m_gasSet,
		m_valueSet,
		m_tokenSet,
		m_saltSet,
		true
	);
}

FunctionTypePointer FunctionType::asExternallyCallableFunction(bool _inLibrary) const
{
	TypePointers parameterTypes;
	for (auto const& t: m_parameterTypes)
		if (TypeProvider::isReferenceWithLocation(t, DataLocation::CallData))
			parameterTypes.push_back(
				TypeProvider::withLocationIfReference(DataLocation::Memory, t, true)
			);
		else
			parameterTypes.push_back(t);

	TypePointers returnParameterTypes;
	for (auto const& returnParamType: m_returnParameterTypes)
		if (TypeProvider::isReferenceWithLocation(returnParamType, DataLocation::CallData))
			returnParameterTypes.push_back(
				TypeProvider::withLocationIfReference(DataLocation::Memory, returnParamType, true)
			);
		else
			returnParameterTypes.push_back(returnParamType);

	Kind kind = m_kind;
	if (_inLibrary)
	{
		solAssert(!!m_declaration, "Declaration has to be available.");
		solAssert(m_declaration->isPublic(), "");
		kind = Kind::DelegateCall;
	}

	return TypeProvider::function(
		parameterTypes,
		returnParameterTypes,
		m_parameterNames,
		m_returnParameterNames,
		kind,
		m_arbitraryParameters,
		m_stateMutability,
		m_declaration,
		m_gasSet,
		m_valueSet,
		m_tokenSet,
		m_saltSet,
		m_bound
	);
}

Type const* FunctionType::selfType() const
{
	solAssert(bound(), "Function is not bound.");
	solAssert(m_parameterTypes.size() > 0, "Function has no self type.");
	return m_parameterTypes.at(0);
}

ASTPointer<StructuredDocumentation> FunctionType::documentation() const
{
	auto function = dynamic_cast<StructurallyDocumented const*>(m_declaration);
	if (function)
		return function->documentation();

	return ASTPointer<StructuredDocumentation>();
}

bool FunctionType::padArguments() const
{
	// No padding only for hash functions, low-level calls and the packed encoding function.
	switch (m_kind)
	{
	case Kind::BareCall:
	case Kind::BareCallCode:
	case Kind::BareDelegateCall:
	case Kind::BareStaticCall:
	case Kind::SHA256:
	case Kind::RIPEMD160:
	case Kind::KECCAK256:
	case Kind::BLAKE2B:
	case Kind::ABIEncodePacked:
		return false;
	default:
		return true;
	}
	return true;
}

Type const* MappingType::encodingType() const
{
	return TypeProvider::integer(256, IntegerType::Modifier::Unsigned);
}

string MappingType::richIdentifier() const
{
	return "t_mapping" + identifierList(m_keyType, m_valueType);
}

bool MappingType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	MappingType const& other = dynamic_cast<MappingType const&>(_other);
	return *other.m_keyType == *m_keyType && *other.m_valueType == *m_valueType;
}

string MappingType::toString(bool _short) const
{
	return "mapping(" + keyType()->toString(_short) + " => " + valueType()->toString(_short) + ")";
}

string MappingType::canonicalName() const
{
	return "mapping(" + keyType()->canonicalName() + " => " + valueType()->canonicalName() + ")";
}

TypeResult MappingType::interfaceType(bool _inLibrary) const
{
	solAssert(keyType()->interfaceType(_inLibrary).get(), "Must be an elementary type!");

	if (_inLibrary)
	{
		auto iType = valueType()->interfaceType(_inLibrary);

		if (!iType.get())
		{
			solAssert(!iType.message().empty(), "Expected detailed error message!");
			return iType;
		}
	}
	else
		return TypeResult::err(
			"Types containing (nested) mappings can only be parameters or "
			"return variables of internal or library functions."
		);

	return this;
}

string TypeType::richIdentifier() const
{
	return "t_type" + identifierList(actualType());
}

bool TypeType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	TypeType const& other = dynamic_cast<TypeType const&>(_other);
	return *actualType() == *other.actualType();
}

u256 TypeType::storageSize() const
{
	solAssert(false, "Storage size of non-storable type type requested.");
}

vector<tuple<string, TypePointer>> TypeType::makeStackItems() const
{
	if (auto contractType = dynamic_cast<ContractType const*>(m_actualType))
		if (contractType->contractDefinition().isLibrary())
		{
			solAssert(!contractType->isSuper(), "");
			return {make_tuple("address", TypeProvider::address())};
		}

	return {};
}

MemberList::MemberMap TypeType::nativeMembers(ASTNode const* _currentScope) const
{
	MemberList::MemberMap members;
	if (m_actualType->category() == Category::Contract)
	{
		auto contractType = dynamic_cast<ContractType const*>(m_actualType);
		ContractDefinition const& contract = contractType->contractDefinition();
		if (contractType->isSuper())
		{
			// add the most derived of all functions which are visible in derived contracts
			auto bases = contract.annotation().linearizedBaseContracts;
			solAssert(bases.size() >= 1, "linearizedBaseContracts should at least contain the most derived contract.");
			// `sliced(1, ...)` ignores the most derived contract, which should not be searchable from `super`.
			for (ContractDefinition const* base: bases | boost::adaptors::sliced(1, bases.size()))
				for (FunctionDefinition const* function: base->definedFunctions())
				{
					if (!function->isVisibleInDerivedContracts() || !function->isImplemented())
						continue;

					auto functionType = TypeProvider::function(*function, FunctionType::Kind::Internal);
					bool functionWithEqualArgumentsFound = false;
					for (auto const& member: members)
					{
						if (member.name != function->name())
							continue;
						auto memberType = dynamic_cast<FunctionType const*>(member.type);
						solAssert(!!memberType, "Override changes type.");
						if (!memberType->hasEqualParameterTypes(*functionType))
							continue;
						functionWithEqualArgumentsFound = true;
						break;
					}
					if (!functionWithEqualArgumentsFound)
						members.emplace_back(function->name(), functionType, function);
				}
		}
		else
		{
			auto const* contractScope = dynamic_cast<ContractDefinition const*>(_currentScope);
			bool inDerivingScope = contractScope && contractScope->derivesFrom(contract);

			for (auto const* declaration: contract.declarations())
			{
				if (dynamic_cast<ModifierDefinition const*>(declaration))
					continue;
				if (declaration->name().empty())
					continue;

				if (!contract.isLibrary() && inDerivingScope && declaration->isVisibleInDerivedContracts())
				{
					if (
						auto const* functionDefinition = dynamic_cast<FunctionDefinition const*>(declaration);
						functionDefinition && !functionDefinition->isImplemented()
					)
						members.emplace_back(declaration->name(), declaration->typeViaContractName(), declaration);
					else
						members.emplace_back(declaration->name(), declaration->type(), declaration);
				}
				else if (
					(contract.isLibrary() && declaration->isVisibleAsLibraryMember()) ||
					declaration->isVisibleViaContractTypeAccess()
				)
					members.emplace_back(declaration->name(), declaration->typeViaContractName(), declaration);
			}
		}
	}
	else if (m_actualType->category() == Category::Enum)
	{
		EnumDefinition const& enumDef = dynamic_cast<EnumType const&>(*m_actualType).enumDefinition();
		auto enumType = TypeProvider::enumType(enumDef);
		for (ASTPointer<EnumValue> const& enumValue: enumDef.members())
			members.emplace_back(enumValue->name(), enumType);
	}
	return members;
}

BoolResult TypeType::isExplicitlyConvertibleTo(Type const& _convertTo) const
{
	if (auto const* address = dynamic_cast<AddressType const*>(&_convertTo))
		if (address->stateMutability() == StateMutability::NonPayable)
			if (auto const* contractType = dynamic_cast<ContractType const*>(m_actualType))
				return contractType->contractDefinition().isLibrary();
	return isImplicitlyConvertibleTo(_convertTo);
}

ModifierType::ModifierType(ModifierDefinition const& _modifier)
{
	TypePointers params;
	params.reserve(_modifier.parameters().size());
	for (ASTPointer<VariableDeclaration> const& var: _modifier.parameters())
		params.push_back(var->annotation().type);
	swap(params, m_parameterTypes);
}

u256 ModifierType::storageSize() const
{
	solAssert(false, "Storage size of non-storable type type requested.");
}

string ModifierType::richIdentifier() const
{
	return "t_modifier" + identifierList(m_parameterTypes);
}

bool ModifierType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	ModifierType const& other = dynamic_cast<ModifierType const&>(_other);

	if (m_parameterTypes.size() != other.m_parameterTypes.size())
		return false;
	auto typeCompare = [](Type const* _a, Type const* _b) -> bool { return *_a == *_b; };

	if (!equal(
		m_parameterTypes.cbegin(),
		m_parameterTypes.cend(),
		other.m_parameterTypes.cbegin(),
		typeCompare
	))
		return false;
	return true;
}

string ModifierType::toString(bool _short) const
{
	string name = "modifier (";
	for (auto it = m_parameterTypes.begin(); it != m_parameterTypes.end(); ++it)
		name += (*it)->toString(_short) + (it + 1 == m_parameterTypes.end() ? "" : ",");
	return name + ")";
}

string ModuleType::richIdentifier() const
{
	return "t_module_" + to_string(m_sourceUnit.id());
}

bool ModuleType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	return &m_sourceUnit == &dynamic_cast<ModuleType const&>(_other).m_sourceUnit;
}

MemberList::MemberMap ModuleType::nativeMembers(ASTNode const*) const
{
	MemberList::MemberMap symbols;
	for (auto const& symbolName: *m_sourceUnit.annotation().exportedSymbols)
		for (Declaration const* symbol: symbolName.second)
			symbols.emplace_back(symbolName.first, symbol->type(), symbol);
	return symbols;
}

string ModuleType::toString(bool) const
{
	return string("module \"") + *m_sourceUnit.annotation().path + string("\"");
}

string MagicType::richIdentifier() const
{
	switch (m_kind)
	{
	case Kind::Block:
		return "t_magic_block";
	case Kind::Message:
		return "t_magic_message";
	case Kind::Transaction:
		return "t_magic_transaction";
	case Kind::ABI:
		return "t_magic_abi";
	case Kind::MetaType:
		solAssert(m_typeArgument, "");
		return "t_magic_meta_type_" + m_typeArgument->richIdentifier();
	}
	return "";
}

bool MagicType::operator==(Type const& _other) const
{
	if (_other.category() != category())
		return false;
	MagicType const& other = dynamic_cast<MagicType const&>(_other);
	return other.m_kind == m_kind;
}

MemberList::MemberMap MagicType::nativeMembers(ASTNode const*) const
{
	switch (m_kind)
	{
	case Kind::Block:
		return MemberList::MemberMap({
			{"coinbase", TypeProvider::payableAddress()},
			{"timestamp", TypeProvider::uint256()},
			{"blockhash", TypeProvider::function(strings{"uint"}, strings{"bytes32"}, FunctionType::Kind::BlockHash, false, StateMutability::View)},
			{"difficulty", TypeProvider::uint256()},
			{"number", TypeProvider::uint256()},
			{"gaslimit", TypeProvider::uint256()},
			{"chainid", TypeProvider::uint256()}
		});
	case Kind::Message:
		return MemberList::MemberMap({
			{"gas", TypeProvider::uint256()},
			{"value", TypeProvider::uint256()},
			{"data", TypeProvider::array(DataLocation::CallData)},
			{"sig", TypeProvider::fixedBytes(4)},
			{"sender", TypeProvider::payableAddress()},  // Solidity++: sender is a payable address
			{"tokenid", TypeProvider::viteTokenId()}, // Solidity++: get tx's transfer token id
			{"amount", TypeProvider::uint256()}  // Solidity++: get tx's transfer amount
		});
	case Kind::Transaction:
		return MemberList::MemberMap({
			{"origin", TypeProvider::address()},
			{"gasprice", TypeProvider::uint256()}
		});
	case Kind::ABI:
		return MemberList::MemberMap({
			{"encode", TypeProvider::function(
				TypePointers{},
				TypePointers{TypeProvider::array(DataLocation::Memory)},
				strings{},
				strings{1, ""},
				FunctionType::Kind::ABIEncode,
				true,
				StateMutability::Pure
			)},
			{"encodePacked", TypeProvider::function(
				TypePointers{},
				TypePointers{TypeProvider::array(DataLocation::Memory)},
				strings{},
				strings{1, ""},
				FunctionType::Kind::ABIEncodePacked,
				true,
				StateMutability::Pure
			)},
			{"encodeWithSelector", TypeProvider::function(
				TypePointers{TypeProvider::fixedBytes(4)},
				TypePointers{TypeProvider::array(DataLocation::Memory)},
				strings{1, ""},
				strings{1, ""},
				FunctionType::Kind::ABIEncodeWithSelector,
				true,
				StateMutability::Pure
			)},
			{"encodeWithSignature", TypeProvider::function(
				TypePointers{TypeProvider::array(DataLocation::Memory, true)},
				TypePointers{TypeProvider::array(DataLocation::Memory)},
				strings{1, ""},
				strings{1, ""},
				FunctionType::Kind::ABIEncodeWithSignature,
				true,
				StateMutability::Pure
			)},
			{"decode", TypeProvider::function(
				TypePointers(),
				TypePointers(),
				strings{},
				strings{},
				FunctionType::Kind::ABIDecode,
				true,
				StateMutability::Pure
			)}
		});
	case Kind::MetaType:
	{
		solAssert(
			m_typeArgument && (
					m_typeArgument->category() == Type::Category::Contract ||
					m_typeArgument->category() == Type::Category::Integer
			),
			"Only contracts or integer types supported for now"
		);

		if (m_typeArgument->category() == Type::Category::Contract)
		{
			ContractDefinition const& contract = dynamic_cast<ContractType const&>(*m_typeArgument).contractDefinition();
			if (contract.canBeDeployed())
				return MemberList::MemberMap({
					{"creationCode", TypeProvider::array(DataLocation::Memory)},
					{"runtimeCode", TypeProvider::array(DataLocation::Memory)},
					{"name", TypeProvider::stringMemory()},
				});
			else
				return MemberList::MemberMap({
					{"interfaceId", TypeProvider::fixedBytes(4)},
					{"name", TypeProvider::stringMemory()},
				});
		}
		else if (m_typeArgument->category() == Type::Category::Integer)
		{
			IntegerType const* integerTypePointer = dynamic_cast<IntegerType const*>(m_typeArgument);
			return MemberList::MemberMap({
				{"min", integerTypePointer},
				{"max", integerTypePointer},
			});
		}
	}
	}
	solAssert(false, "Unknown kind of magic.");
	return {};
}

string MagicType::toString(bool _short) const
{
	switch (m_kind)
	{
	case Kind::Block:
		return "block";
	case Kind::Message:
		return "msg";
	case Kind::Transaction:
		return "tx";
	case Kind::ABI:
		return "abi";
	case Kind::MetaType:
		solAssert(m_typeArgument, "");
		return "type(" + m_typeArgument->toString(_short) + ")";
	}
	solAssert(false, "Unknown kind of magic.");
	return {};
}

TypePointer MagicType::typeArgument() const
{
	solAssert(m_kind == Kind::MetaType, "");
	solAssert(m_typeArgument, "");
	return m_typeArgument;
}

TypePointer InaccessibleDynamicType::decodingType() const
{
	return TypeProvider::integer(256, IntegerType::Modifier::Unsigned);
}
