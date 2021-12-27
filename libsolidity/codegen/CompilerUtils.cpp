// SPDX-License-Identifier: GPL-3.0
/**
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Routines used by both the compiler and the expression compiler.
 */

#include <libsolidity/codegen/CompilerUtils.h>

#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/TypeProvider.h>
#include <libsolidity/codegen/ABIFunctions.h>
#include <libsolidity/codegen/ArrayUtils.h>
#include <libsolidity/codegen/LValue.h>
#include <libsolutil/FunctionSelector.h>
#include <libevmasm/Instruction.h>
#include <libsolutil/Whiskers.h>

using namespace std;
using namespace solidity;
using namespace solidity::evmasm;
using namespace solidity::frontend;
using namespace solidity::langutil;

using solidity::util::Whiskers;
using solidity::util::h256;
using solidity::util::toCompactHexWithPrefix;

unsigned const CompilerUtils::dataStartOffset = 4;
size_t const CompilerUtils::freeMemoryPointer = 64;
size_t const CompilerUtils::zeroPointer = CompilerUtils::freeMemoryPointer + 32;
size_t const CompilerUtils::generalPurposeMemoryStart = CompilerUtils::zeroPointer + 32;

static_assert(CompilerUtils::freeMemoryPointer >= 64, "Free memory pointer must not overlap with scratch area.");
static_assert(CompilerUtils::zeroPointer >= CompilerUtils::freeMemoryPointer + 32, "Zero pointer must not overlap with free memory pointer.");
static_assert(CompilerUtils::generalPurposeMemoryStart >= CompilerUtils::zeroPointer + 32, "General purpose memory must not overlap with zero area.");

void CompilerUtils::initialiseFreeMemoryPointer()
{
	size_t reservedMemory = m_context.reservedMemory();
	solAssert(bigint(generalPurposeMemoryStart) + bigint(reservedMemory) < bigint(1) << 63, "");
	m_context << (u256(generalPurposeMemoryStart) + reservedMemory);
	storeFreeMemoryPointer();
}

void CompilerUtils::fetchFreeMemoryPointer()
{
    m_context.appendDebugInfo("CompilerUtils::fetchFreeMemoryPointer()");
	m_context << u256(freeMemoryPointer) << Instruction::MLOAD;
    m_context.appendDebugInfo("end of CompilerUtils::fetchFreeMemoryPointer()");
}

void CompilerUtils::storeFreeMemoryPointer()
{
    m_context.appendDebugInfo("CompilerUtils::storeFreeMemoryPointer()");
	m_context << u256(freeMemoryPointer) << Instruction::MSTORE;
    m_context.appendDebugInfo("end of CompilerUtils::storeFreeMemoryPointer()");
}

void CompilerUtils::allocateMemory()
{
    m_context.appendDebugInfo("CompilerUtils::allocateMemory()");
	fetchFreeMemoryPointer();
	m_context << Instruction::SWAP1 << Instruction::DUP2 << Instruction::ADD;
	storeFreeMemoryPointer();
    m_context.appendDebugInfo("end of CompilerUtils::allocateMemory()");
}

void CompilerUtils::allocateMemory(u256 const& size)
{
	fetchFreeMemoryPointer();
	m_context << Instruction::DUP1 << size << Instruction::ADD;
	storeFreeMemoryPointer();
}

void CompilerUtils::toSizeAfterFreeMemoryPointer()
{
	fetchFreeMemoryPointer();
	m_context << Instruction::DUP1 << Instruction::SWAP2 << Instruction::SUB;
	m_context << Instruction::SWAP1;
}

void CompilerUtils::revertWithStringData(Type const& _argumentType)
{
	solAssert(_argumentType.isImplicitlyConvertibleTo(*TypeProvider::fromElementaryTypeName("string memory")), "");
	fetchFreeMemoryPointer();
	m_context << util::selectorFromSignature("Error(string)");
	m_context << Instruction::DUP2 << Instruction::MSTORE;
	m_context << u256(4) << Instruction::ADD;
	// Stack: <string data> <mem pos of encoding start>
	abiEncode({&_argumentType}, {TypeProvider::array(DataLocation::Memory, true)});
	toSizeAfterFreeMemoryPointer();
	m_context << Instruction::REVERT;
}

void CompilerUtils::returnDataToArray()
{
	if (m_context.evmVersion().supportsReturndata())
	{
		m_context << Instruction::RETURNDATASIZE;
		m_context.appendInlineAssembly(R"({
			switch v case 0 {
				v := 0x60
			} default {
				v := mload(0x40)
				mstore(0x40, add(v, and(add(returndatasize(), 0x3f), not(0x1f))))
				mstore(v, returndatasize())
				returndatacopy(add(v, 0x20), 0, returndatasize())
			}
		})", {"v"});
	}
	else
		pushZeroPointer();
}

void CompilerUtils::accessCalldataTail(Type const& _type)
{
	m_context << Instruction::SWAP1;
	m_context.callYulFunction(
		m_context.utilFunctions().accessCalldataTailFunction(_type),
		2,
		_type.isDynamicallySized() ? 2 : 1
	);
}

unsigned CompilerUtils::loadFromMemory(
	unsigned _offset,
	Type const& _type,
	bool _fromCalldata,
	bool _padToWordBoundaries
)
{
	solAssert(_type.category() != Type::Category::Array, "Unable to statically load dynamic type.");
	m_context.appendDebugInfo("CompilerUtils::loadFromMemory("
                              "offset=" + to_string(_offset) +
                              ", type=" + _type.toString(false) +
                              ", fromCalldata=" + to_string(_fromCalldata) +
                              ", padToWordBoundaries=" + to_string(_padToWordBoundaries) +
                              ")");

	m_context << u256(_offset);
	return loadFromMemoryHelper(_type, _fromCalldata, _padToWordBoundaries);
}

void CompilerUtils::loadFromMemoryDynamic(
	Type const& _type,
	bool _fromCalldata,
	bool _padToWordBoundaries,
	bool _keepUpdatedMemoryOffset
)
{
	if (_keepUpdatedMemoryOffset)
		m_context << Instruction::DUP1;

	if (auto arrayType = dynamic_cast<ArrayType const*>(&_type))
	{
		solAssert(!arrayType->isDynamicallySized(), "");
		solAssert(!_fromCalldata, "");
		solAssert(_padToWordBoundaries, "");
		if (_keepUpdatedMemoryOffset)
			m_context << arrayType->memoryDataSize() << Instruction::ADD;
	}
	else
	{
		unsigned numBytes = loadFromMemoryHelper(_type, _fromCalldata, _padToWordBoundaries);
		if (_keepUpdatedMemoryOffset)
		{
			// update memory counter
			moveToStackTop(_type.sizeOnStack());
			m_context << u256(numBytes) << Instruction::ADD;
		}
	}
}

void CompilerUtils::storeInMemory(unsigned _offset)
{
    m_context.appendDebugInfo("CompilerUtils::storeInMemory(offset=" + to_string(_offset) +")");
	unsigned numBytes = prepareMemoryStore(*TypeProvider::uint256(), true);
	if (numBytes > 0)
		m_context << u256(_offset) << Instruction::MSTORE;
	m_context.appendDebugInfo("end of CompilerUtils::storeInMemory()");
}

void CompilerUtils::storeInMemoryDynamic(Type const& _type, bool _padToWordBoundaries, bool _cleanup)
{
	// process special types (Reference, StringLiteral, Function)
	auto comment = "CompilerUtils::storeInMemoryDynamic(type=" + _type.toString(false) + ", _padToWordBoundaries=" + to_string(_padToWordBoundaries) + ", _cleanup=" + to_string(_cleanup) + ")";
	m_context.appendDebugInfo(comment);
	if (auto ref = dynamic_cast<ReferenceType const*>(&_type))
	{
		solUnimplementedAssert(
			ref->location() == DataLocation::Memory,
			"Only in-memory reference type can be stored."
		);
		storeInMemoryDynamic(*TypeProvider::uint256(), _padToWordBoundaries, _cleanup);
	}
	else if (auto str = dynamic_cast<StringLiteralType const*>(&_type))
	{
		m_context << Instruction::DUP1;
		storeStringData(bytesConstRef(str->value()));
		if (_padToWordBoundaries)
			m_context << u256(max<size_t>(32, ((str->value().size() + 31) / 32) * 32));
		else
			m_context << u256(str->value().size());
		m_context << Instruction::ADD;
	}
	else if (
		_type.category() == Type::Category::Function &&
		dynamic_cast<FunctionType const&>(_type).kind() == FunctionType::Kind::External
	)
	{
		combineExternalFunctionType(true);
		m_context << Instruction::DUP2 << Instruction::MSTORE;
		m_context << u256(_padToWordBoundaries ? 32 : 24) << Instruction::ADD;
	}
	else if (_type.isValueType())
	{
		unsigned numBytes = prepareMemoryStore(_type, _padToWordBoundaries, _cleanup);
		m_context << Instruction::DUP2 << Instruction::MSTORE;
		m_context.appendDebugInfo("bytes of type " + _type.toString(false) + ": " + to_string(numBytes));
		m_context << u256(numBytes) << Instruction::ADD;
	}
	else // Should never happen
	{
		solAssert(
			false,
			"Memory store of type " + _type.toString(true) + " not allowed."
		);
	}
	m_context.appendDebugInfo("end of CompilerUtils::storeInMemoryDynamic()");
}

void CompilerUtils::abiDecode(TypePointers const& _typeParameters, bool _fromMemory)
{
	/// Stack: <source_offset> <length>
	if (m_context.useABICoderV2())
	{
		// Use the new Yul-based decoding function
		auto stackHeightBefore = m_context.stackHeight();
		abiDecodeV2(_typeParameters, _fromMemory);
		solAssert(m_context.stackHeight() - stackHeightBefore == sizeOnStack(_typeParameters) - 2, "");
		return;
	}

	//@todo this does not yet support nested dynamic arrays
	size_t encodedSize = 0;
	for (auto const& t: _typeParameters)
		encodedSize += t->decodingType()->calldataHeadSize();

	Whiskers templ(R"({
		if lt(len, <encodedSize>) { <revertString> }
	})");
	templ("encodedSize", to_string(encodedSize));
	templ("revertString", m_context.revertReasonIfDebug("Calldata too short"));
	m_context.appendInlineAssembly(templ.render(), {"len"});

	m_context << Instruction::DUP2 << Instruction::ADD;
	m_context << Instruction::SWAP1;
	/// Stack: <input_end> <source_offset>

	// Retain the offset pointer as base_offset, the point from which the data offsets are computed.
	m_context << Instruction::DUP1;
	for (TypePointer const& parameterType: _typeParameters)
	{
		// stack: v1 v2 ... v(k-1) input_end base_offset current_offset
		TypePointer type = parameterType->decodingType();
		solUnimplementedAssert(type, "No decoding type found.");
		if (type->category() == Type::Category::Array)
		{
			auto const& arrayType = dynamic_cast<ArrayType const&>(*type);
			solUnimplementedAssert(!arrayType.baseType()->isDynamicallyEncoded(), "Nested arrays not yet implemented.");
			if (_fromMemory)
			{
				solUnimplementedAssert(
					arrayType.baseType()->isValueType(),
					"Nested memory arrays not yet implemented here."
				);
				// @todo If base type is an array or struct, it is still calldata-style encoded, so
				// we would have to convert it like below.
				solAssert(arrayType.location() == DataLocation::Memory, "");
				if (arrayType.isDynamicallySized())
				{
					// compute data pointer
					m_context << Instruction::DUP1 << Instruction::MLOAD;
					// stack: v1 v2 ... v(k-1) input_end base_offset current_offset data_offset

					fetchFreeMemoryPointer();
					// stack: v1 v2 ... v(k-1) input_end base_offset current_offset data_offset dstmem
					moveIntoStack(4);
					// stack: v1 v2 ... v(k-1) dstmem input_end base_offset current_offset data_offset
					m_context << Instruction::DUP5;
					// stack: v1 v2 ... v(k-1) dstmem input_end base_offset current_offset data_offset dstmem

					// Check that the data pointer is valid and that length times
					// item size is still inside the range.
					Whiskers templ(R"({
						if gt(ptr, 0x100000000) { <revertStringPointer> }
						ptr := add(ptr, base_offset)
						let array_data_start := add(ptr, 0x20)
						if gt(array_data_start, input_end) { <revertStringStart> }
						let array_length := mload(ptr)
						if or(
							gt(array_length, 0x100000000),
							gt(add(array_data_start, mul(array_length, <item_size>)), input_end)
						) { <revertStringLength> }
						mstore(dst, array_length)
						dst := add(dst, 0x20)
					})");
					templ("item_size", to_string(arrayType.calldataStride()));
					// TODO add test
					templ("revertStringPointer", m_context.revertReasonIfDebug("ABI memory decoding: invalid data pointer"));
					templ("revertStringStart", m_context.revertReasonIfDebug("ABI memory decoding: invalid data start"));
					templ("revertStringLength", m_context.revertReasonIfDebug("ABI memory decoding: invalid data length"));
					m_context.appendInlineAssembly(templ.render(), {"input_end", "base_offset", "offset", "ptr", "dst"});
					// stack: v1 v2 ... v(k-1) dstmem input_end base_offset current_offset data_ptr dstdata
					m_context << Instruction::SWAP1;
					// stack: v1 v2 ... v(k-1) dstmem input_end base_offset current_offset dstdata data_ptr
					ArrayUtils(m_context).copyArrayToMemory(arrayType, true);
					// stack: v1 v2 ... v(k-1) dstmem input_end base_offset current_offset mem_end
					storeFreeMemoryPointer();
					m_context << u256(0x20) << Instruction::ADD;
				}
				else
				{
					// Size has already been checked for this one.
					moveIntoStack(2);
					m_context << Instruction::DUP3;
					m_context << u256(arrayType.calldataHeadSize()) << Instruction::ADD;
				}
			}
			else
			{
				// first load from calldata and potentially convert to memory if arrayType is memory
				TypePointer calldataType = TypeProvider::withLocation(&arrayType, DataLocation::CallData, false);
				if (calldataType->isDynamicallySized())
				{
					// put on stack: data_pointer length
					loadFromMemoryDynamic(*TypeProvider::uint256(), !_fromMemory);
					m_context << Instruction::SWAP1;
					// stack: input_end base_offset next_pointer data_offset
					m_context.appendInlineAssembly(Whiskers(R"({
						if gt(data_offset, 0x100000000) { <revertString> }
					})")
					// TODO add test
					("revertString", m_context.revertReasonIfDebug("ABI calldata decoding: invalid data offset"))
					.render(), {"data_offset"});
					m_context << Instruction::DUP3 << Instruction::ADD;
					// stack: input_end base_offset next_pointer array_head_ptr
					m_context.appendInlineAssembly(Whiskers(R"({
						if gt(add(array_head_ptr, 0x20), input_end) { <revertString> }
					})")
					("revertString", m_context.revertReasonIfDebug("ABI calldata decoding: invalid head pointer"))
					.render(), {"input_end", "base_offset", "next_ptr", "array_head_ptr"});

					// retrieve length
					loadFromMemoryDynamic(*TypeProvider::uint256(), !_fromMemory, true);
					// stack: input_end base_offset next_pointer array_length data_pointer
					m_context << Instruction::SWAP2;
					// stack: input_end base_offset data_pointer array_length next_pointer
					m_context.appendInlineAssembly(Whiskers(R"({
						if or(
							gt(array_length, 0x100000000),
							gt(add(data_ptr, mul(array_length, )" + to_string(arrayType.calldataStride()) + R"()), input_end)
						) { <revertString> }
					})")
					("revertString", m_context.revertReasonIfDebug("ABI calldata decoding: invalid data pointer"))
					.render(), {"input_end", "base_offset", "data_ptr", "array_length", "next_ptr"});
				}
				else
				{
					// size has already been checked
					// stack: input_end base_offset data_offset
					m_context << Instruction::DUP1;
					m_context << u256(calldataType->calldataHeadSize()) << Instruction::ADD;
				}
				if (arrayType.location() == DataLocation::Memory)
				{
					// stack: input_end base_offset calldata_ref [length] next_calldata
					// copy to memory
					// move calldata type up again
					moveIntoStack(calldataType->sizeOnStack());
					convertType(*calldataType, arrayType, false, false, true);
					// fetch next pointer again
					moveToStackTop(arrayType.sizeOnStack());
				}
				// move input_end up
				// stack: input_end base_offset calldata_ref [length] next_calldata
				moveToStackTop(2 + arrayType.sizeOnStack());
				m_context << Instruction::SWAP1;
				// stack: base_offset calldata_ref [length] input_end next_calldata
				moveToStackTop(2 + arrayType.sizeOnStack());
				m_context << Instruction::SWAP1;
				// stack: calldata_ref [length] input_end base_offset next_calldata
			}
		}
		else
		{
			solAssert(!type->isDynamicallyEncoded(), "Unknown dynamically sized type: " + type->toString());
			loadFromMemoryDynamic(*type, !_fromMemory, true);
			// stack: v1 v2 ... v(k-1) input_end base_offset v(k) mem_offset
			moveToStackTop(1, type->sizeOnStack());
			moveIntoStack(3, type->sizeOnStack());
		}
		// stack: v1 v2 ... v(k-1) v(k) input_end base_offset next_offset
	}
	popStackSlots(3);
}

void CompilerUtils::encodeToMemory(
	TypePointers const& _givenTypes,
	TypePointers const& _targetTypes,
	bool _padToWordBoundaries,
	bool _copyDynamicDataInPlace,
	bool _encodeAsLibraryTypes
)
{
    m_context.appendDebugInfo("CompilerUtils::encodeToMemory()  stack[<v1> <v2> ... <vn> <mem>]");
    m_context.appendDebugInfo("    givenTypes: [");
    for(auto t : _givenTypes)
        m_context.appendDebugInfo("            " + t->toString(false));
    m_context.appendDebugInfo("            ]");

	// stack: <v1> <v2> ... <vn> <mem>
	bool const encoderV2 = m_context.useABICoderV2();
	TypePointers targetTypes = _targetTypes.empty() ? _givenTypes : _targetTypes;
	solAssert(targetTypes.size() == _givenTypes.size(), "");
	for (TypePointer& t: targetTypes)
	{
		TypePointer tEncoding = t->fullEncodingType(_encodeAsLibraryTypes, encoderV2, !_padToWordBoundaries);
		solUnimplementedAssert(tEncoding, "Encoding type \"" + t->toString() + "\" not yet implemented.");
		t = std::move(tEncoding);
	}

	if (_givenTypes.empty())
		return;
	if (encoderV2)
	{
		// Use the new Yul-based encoding function
		solAssert(
			_padToWordBoundaries != _copyDynamicDataInPlace,
			"Non-padded and in-place encoding can only be combined."
		);
		auto stackHeightBefore = m_context.stackHeight();
		abiEncodeV2(_givenTypes, targetTypes, _encodeAsLibraryTypes, _padToWordBoundaries);
		solAssert(stackHeightBefore - m_context.stackHeight() == sizeOnStack(_givenTypes), "");
		return;
	}

	// Stack during operation:
	// <v1> <v2> ... <vn> <mem_start> <dyn_head_1> ... <dyn_head_r> <end_of_mem>
	// The values dyn_head_n are added during the first loop and they point to the head part
	// of the nth dynamic parameter, which is filled once the dynamic parts are processed.

	// store memory start pointer
	m_context << Instruction::DUP1;

	unsigned argSize = CompilerUtils::sizeOnStack(_givenTypes);
	unsigned stackPos = 0; // advances through the argument values
	unsigned dynPointers = 0; // number of dynamic head pointers on the stack
	for (size_t i = 0; i < _givenTypes.size(); ++i)
	{
		TypePointer targetType = targetTypes[i];
		solAssert(!!targetType, "Externalable type expected.");
		if (targetType->isDynamicallySized() && !_copyDynamicDataInPlace)
		{
			// leave end_of_mem as dyn head pointer
			m_context << Instruction::DUP1 << u256(32) << Instruction::ADD;
			dynPointers++;
			assertThrow(
				(argSize + dynPointers) < 16,
				StackTooDeepError,
				"Stack too deep, try using fewer variables."
			);
		}
		else
		{
			bool needCleanup = true;
			copyToStackTop(argSize - stackPos + dynPointers + 2, _givenTypes[i]->sizeOnStack());
			solAssert(!!targetType, "Externalable type expected.");
			TypePointer type = targetType;
			if (_givenTypes[i]->dataStoredIn(DataLocation::Storage) && targetType->isValueType())
			{
				// special case: convert storage reference type to value type - this is only
				// possible for library calls where we just forward the storage reference
				solAssert(_encodeAsLibraryTypes, "");
				solAssert(_givenTypes[i]->sizeOnStack() == 1, "");
			}
			else if (
				_givenTypes[i]->dataStoredIn(DataLocation::Storage) ||
				_givenTypes[i]->dataStoredIn(DataLocation::CallData) ||
				_givenTypes[i]->category() == Type::Category::StringLiteral ||
				_givenTypes[i]->category() == Type::Category::Function
			)
				type = _givenTypes[i]; // delay conversion
			else
			{
				convertType(*_givenTypes[i], *targetType, true);
				needCleanup = false;
			}

			if (auto arrayType = dynamic_cast<ArrayType const*>(type))
				ArrayUtils(m_context).copyArrayToMemory(*arrayType, _padToWordBoundaries);
			else if (auto arraySliceType = dynamic_cast<ArraySliceType const*>(type))
			{
				solAssert(
					arraySliceType->dataStoredIn(DataLocation::CallData) &&
					arraySliceType->isDynamicallySized() &&
					!arraySliceType->arrayType().baseType()->isDynamicallyEncoded(),
					""
				);
				ArrayUtils(m_context).copyArrayToMemory(arraySliceType->arrayType(), _padToWordBoundaries);
			}
			else
				storeInMemoryDynamic(*type, _padToWordBoundaries, needCleanup);
		}
		stackPos += _givenTypes[i]->sizeOnStack();
	}

	// now copy the dynamic part
	// Stack: <v1> <v2> ... <vn> <mem_start> <dyn_head_1> ... <dyn_head_r> <end_of_mem>
	stackPos = 0;
	unsigned thisDynPointer = 0;
	for (size_t i = 0; i < _givenTypes.size(); ++i)
	{
		TypePointer targetType = targetTypes[i];
		solAssert(!!targetType, "Externalable type expected.");
		if (targetType->isDynamicallySized() && !_copyDynamicDataInPlace)
		{
			// copy tail pointer (=mem_end - mem_start) to memory
			assertThrow(
				(2 + dynPointers) <= 16,
				StackTooDeepError,
				"Stack too deep(" + to_string(2 + dynPointers) + "), try using fewer variables."
			);
			m_context << dupInstruction(2 + dynPointers) << Instruction::DUP2;
			m_context << Instruction::SUB;
			m_context << dupInstruction(2 + dynPointers - thisDynPointer);
			m_context << Instruction::MSTORE;
			// stack: ... <end_of_mem>
			if (_givenTypes[i]->category() == Type::Category::StringLiteral)
			{
				auto const& strType = dynamic_cast<StringLiteralType const&>(*_givenTypes[i]);
				auto const size = strType.value().size();
				m_context << u256(size);
				storeInMemoryDynamic(*TypeProvider::uint256(), true);
				// stack: ... <end_of_mem'>

				// Do not output empty padding for zero-length strings.
				// TODO: handle this in storeInMemoryDynamic
				if (size != 0)
					storeInMemoryDynamic(strType, _padToWordBoundaries);
			}
			else
			{
				ArrayType const* arrayType = nullptr;
				switch (_givenTypes[i]->category())
				{
					case Type::Category::Array:
						arrayType = dynamic_cast<ArrayType const*>(_givenTypes[i]);
						break;
					case Type::Category::ArraySlice:
						arrayType = &dynamic_cast<ArraySliceType const*>(_givenTypes[i])->arrayType();
						solAssert(
							arrayType->isDynamicallySized() &&
							arrayType->dataStoredIn(DataLocation::CallData) &&
							!arrayType->baseType()->isDynamicallyEncoded(),
							""
						);
						break;
					default:
						solAssert(false, "Unknown dynamic type.");
						break;
				}
				// now copy the array
				copyToStackTop(argSize - stackPos + dynPointers + 2, arrayType->sizeOnStack());
				// stack: ... <end_of_mem> <value...>
				// copy length to memory
				m_context << dupInstruction(1 + arrayType->sizeOnStack());
				ArrayUtils(m_context).retrieveLength(*arrayType, 1);
				// stack: ... <end_of_mem> <value...> <end_of_mem'> <length>
				storeInMemoryDynamic(*TypeProvider::uint256(), true);
				// stack: ... <end_of_mem> <value...> <end_of_mem''>
				// copy the new memory pointer
				m_context << swapInstruction(arrayType->sizeOnStack() + 1) << Instruction::POP;
				// stack: ... <end_of_mem''> <value...>
				// copy data part
				ArrayUtils(m_context).copyArrayToMemory(*arrayType, _padToWordBoundaries);
				// stack: ... <end_of_mem'''>
			}

			thisDynPointer++;
		}
		stackPos += _givenTypes[i]->sizeOnStack();
	}

	// remove unneeded stack elements (and retain memory pointer)
	m_context << swapInstruction(argSize + dynPointers + 1);
	popStackSlots(argSize + dynPointers + 1);
	m_context.appendDebugInfo("end of encodeToMemory");
}

void CompilerUtils::abiEncodeV2(
	TypePointers const& _givenTypes,
	TypePointers const& _targetTypes,
	bool _encodeAsLibraryTypes,
	bool _padToWordBoundaries
)
{
	if (!_padToWordBoundaries)
		solAssert(!_encodeAsLibraryTypes, "Library calls cannot be packed.");

	// stack: <$value0> <$value1> ... <$value(n-1)> <$headStart>
	m_context.appendDebugInfo("CompilerUtils::abiEncodeV2()  stack[<$value0> <$value1> ... <$value(n-1)> <$headStart>] top");
	string encoderName =
		_padToWordBoundaries ?
		m_context.abiFunctions().tupleEncoderReversed(_givenTypes, _targetTypes, _encodeAsLibraryTypes) :
		m_context.abiFunctions().tupleEncoderPackedReversed(_givenTypes, _targetTypes);
	m_context.callYulFunction(encoderName, sizeOnStack(_givenTypes) + 1, 1);
}

void CompilerUtils::abiDecodeV2(TypePointers const& _parameterTypes, bool _fromMemory)
{
	// stack: <source_offset> <length> [stack top]
	m_context.appendDebugInfo("CompilerUtils::abiDecodeV2() stack[<source_offset> <length>] top");
	m_context << Instruction::DUP2 << Instruction::ADD;
	m_context << Instruction::SWAP1;
	// stack: <end> <start>
	m_context.appendDebugInfo("CompilerUtils::abiDecodeV2() stack[<end> <start>] top");
	string decoderName = m_context.abiFunctions().tupleDecoder(_parameterTypes, _fromMemory);
	m_context.callYulFunction(decoderName, 2, sizeOnStack(_parameterTypes));
}

void CompilerUtils::zeroInitialiseMemoryArray(ArrayType const& _type)
{
	if (_type.baseType()->hasSimpleZeroValueInMemory())
	{
		solAssert(_type.baseType()->isValueType(), "");
		Whiskers templ(R"({
			let size := mul(length, <element_size>)
			// cheap way of zero-initializing a memory range
			calldatacopy(memptr, calldatasize(), size)
			memptr := add(memptr, size)
		})");
		templ("element_size", to_string(_type.memoryStride()));
		m_context.appendInlineAssembly(templ.render(), {"length", "memptr"});
	}
	else
	{
		auto repeat = m_context.newTag();
		m_context << repeat;
		pushZeroValue(*_type.baseType());
		storeInMemoryDynamic(*_type.baseType());
		m_context << Instruction::SWAP1 << u256(1) << Instruction::SWAP1;
		m_context << Instruction::SUB << Instruction::SWAP1;
		m_context << Instruction::DUP2;
		m_context.appendConditionalJumpTo(repeat);
	}
	m_context << Instruction::SWAP1 << Instruction::POP;
}

void CompilerUtils::memoryCopy32()
{
	// Stack here: size target source

	m_context.appendInlineAssembly(R"(
		{
			for { let i := 0 } lt(i, len) { i := add(i, 32) } {
				mstore(add(dst, i), mload(add(src, i)))
			}
		}
	)",
		{ "len", "dst", "src" }
	);
	m_context << Instruction::POP << Instruction::POP << Instruction::POP;
}

void CompilerUtils::memoryCopy()
{
	// Stack here: size target source

	m_context.appendInlineAssembly(R"(
		{
			// copy 32 bytes at once
			for
				{}
				iszero(lt(len, 32))
				{
					dst := add(dst, 32)
					src := add(src, 32)
					len := sub(len, 32)
				}
				{ mstore(dst, mload(src)) }

			// copy the remainder (0 < len < 32)
			let mask := sub(exp(256, sub(32, len)), 1)
			let srcpart := and(mload(src), not(mask))
			let dstpart := and(mload(dst), mask)
			mstore(dst, or(srcpart, dstpart))
		}
	)",
		{ "len", "dst", "src" }
	);
	m_context << Instruction::POP << Instruction::POP << Instruction::POP;
}

void CompilerUtils::splitExternalFunctionType(bool _leftAligned)
{
	// We have to split the left-aligned <address><function identifier> into two stack slots:
	// address (right aligned), function identifier (right aligned)
	if (_leftAligned)
	{
		m_context << Instruction::DUP1;
		rightShiftNumberOnStack(64 + 32);
		// <input> <address>
		m_context << Instruction::SWAP1;
		rightShiftNumberOnStack(64);
	}
	else
	{
		m_context << Instruction::DUP1;
		rightShiftNumberOnStack(32);
		m_context << ((u256(1) << 168) - 1) << Instruction::AND << Instruction::SWAP1;  // Solidity++: 168-bit address
	}
	m_context << u256(0xffffffffUL) << Instruction::AND;
}

void CompilerUtils::combineExternalFunctionType(bool _leftAligned)
{
	// <address> <function_id>
	m_context.appendDebugInfo("CompilerUtils::combineExternalFunctionType(leftAligned=" + to_string(_leftAligned) + ")");
	m_context << u256(0xffffffffUL) << Instruction::AND << Instruction::SWAP1;
	if (!_leftAligned)
		m_context << ((u256(1) << 168) - 1)  << Instruction::AND;  // Solidity++: 168-bit address
	leftShiftNumberOnStack(32);
	m_context << Instruction::OR;
	if (_leftAligned)
		leftShiftNumberOnStack(64);
	m_context.appendDebugInfo("end of CompilerUtils::combineExternalFunctionType()");
}

void CompilerUtils::pushCombinedFunctionEntryLabel(Declaration const& _function, bool _runtimeOnly)
{
	m_context << m_context.functionEntryLabel(_function).pushTag();
	// If there is a runtime context, we have to merge both labels into the same
	// stack slot in case we store it in storage.
	if (CompilerContext* rtc = m_context.runtimeContext())
	{
		leftShiftNumberOnStack(32);
		if (_runtimeOnly)
			m_context <<
				rtc->functionEntryLabel(_function).toSubAssemblyTag(m_context.runtimeSub()) <<
				Instruction::OR;
	}
}

void CompilerUtils::convertType(
	Type const& _typeOnStack,
	Type const& _targetType,
	bool _cleanupNeeded,
	bool _chopSignBits,
	bool _asPartOfArgumentDecoding
)
{
	// For a type extension, we need to remove all higher-order bits that we might have ignored in
	// previous operations.
	// @todo: store in the AST whether the operand might have "dirty" higher order bits

	if (_typeOnStack == _targetType && !_cleanupNeeded)
		return;

    m_context.appendDebugInfo("CompilerUtils::convertType(): " + _typeOnStack.toString(false) + " -> " + _targetType.toString(false));

	Type::Category stackTypeCategory = _typeOnStack.category();
	Type::Category targetTypeCategory = _targetType.category();

	if (auto contrType = dynamic_cast<ContractType const*>(&_typeOnStack))
		solAssert(!contrType->isSuper(), "Cannot convert magic variable \"super\"");

	bool enumOverflowCheckPending = (targetTypeCategory == Type::Category::Enum || stackTypeCategory == Type::Category::Enum);
	bool chopSignBitsPending = _chopSignBits && targetTypeCategory == Type::Category::Integer;
	if (chopSignBitsPending)
	{
		IntegerType const& targetIntegerType = dynamic_cast<IntegerType const&>(_targetType);
		chopSignBitsPending = targetIntegerType.isSigned();
	}

	if (targetTypeCategory == Type::Category::FixedPoint)
		solUnimplemented("Not yet implemented - FixedPointType.");

	switch (stackTypeCategory)
	{
	case Type::Category::FixedBytes:
	{
		FixedBytesType const& typeOnStack = dynamic_cast<FixedBytesType const&>(_typeOnStack);
		if (targetTypeCategory == Type::Category::Integer)
		{
			// conversion from bytes to integer. no need to clean the high bit
			// only to shift right because of opposite alignment
			IntegerType const& targetIntegerType = dynamic_cast<IntegerType const&>(_targetType);
			rightShiftNumberOnStack(256 - typeOnStack.numBytes() * 8);
			if (targetIntegerType.numBits() < typeOnStack.numBytes() * 8)
				convertType(IntegerType(typeOnStack.numBytes() * 8), _targetType, _cleanupNeeded);
		}
		else if (targetTypeCategory == Type::Category::Address)
		{
			// Solidity++: 168-bit address
			solAssert(typeOnStack.numBytes() * 8 == 168, "");
			rightShiftNumberOnStack(256 - 168);
		}
		else
		{
			// clear for conversion to longer bytes
			solAssert(targetTypeCategory == Type::Category::FixedBytes, "Invalid type conversion requested.");
			FixedBytesType const& targetType = dynamic_cast<FixedBytesType const&>(_targetType);
			if (typeOnStack.numBytes() == 0 || targetType.numBytes() == 0)
				m_context << Instruction::POP << u256(0);
			else if (targetType.numBytes() > typeOnStack.numBytes() || _cleanupNeeded)
			{
				unsigned bytes = min(typeOnStack.numBytes(), targetType.numBytes());
				m_context << ((u256(1) << (256 - bytes * 8)) - 1);
				m_context << Instruction::NOT << Instruction::AND;
			}
		}
		break;
	}
	case Type::Category::Enum:
		solAssert(_targetType == _typeOnStack || targetTypeCategory == Type::Category::Integer, "");
		if (enumOverflowCheckPending)
		{
			EnumType const& enumType = dynamic_cast<decltype(enumType)>(_typeOnStack);
			solAssert(enumType.numberOfMembers() > 0, "empty enum should have caused a parser error.");
			m_context << u256(enumType.numberOfMembers() - 1) << Instruction::DUP2 << Instruction::GT;
			if (_asPartOfArgumentDecoding)
				m_context.appendConditionalRevert(false, "Enum out of range");
			else
				m_context.appendConditionalPanic(util::PanicCode::EnumConversionError);
			enumOverflowCheckPending = false;
		}
		break;
	case Type::Category::FixedPoint:
		solUnimplemented("Not yet implemented - FixedPointType.");
	case Type::Category::Address:
	case Type::Category::Integer:
	case Type::Category::Contract:
	case Type::Category::RationalNumber:
		if (targetTypeCategory == Type::Category::FixedBytes)
		{
			solAssert(
				stackTypeCategory == Type::Category::Address ||
				stackTypeCategory == Type::Category::Integer ||
				stackTypeCategory == Type::Category::RationalNumber,
				"Invalid conversion to FixedBytesType requested."
			);
			// conversion from bytes to string. no need to clean the high bit
			// only to shift left because of opposite alignment
			FixedBytesType const& targetBytesType = dynamic_cast<FixedBytesType const&>(_targetType);
			if (auto typeOnStack = dynamic_cast<IntegerType const*>(&_typeOnStack))
			{
				if (targetBytesType.numBytes() * 8 > typeOnStack->numBits())
					cleanHigherOrderBits(*typeOnStack);
			}
			else if (stackTypeCategory == Type::Category::Address)
				solAssert(targetBytesType.numBytes() * 8 == 168, "");  // Solidity++: 168-bit address
			leftShiftNumberOnStack(256 - targetBytesType.numBytes() * 8);
		}
		else if (targetTypeCategory == Type::Category::Enum)
		{
			solAssert(stackTypeCategory != Type::Category::Address, "Invalid conversion to EnumType requested.");
			solAssert(_typeOnStack.mobileType(), "");
			// just clean
			convertType(_typeOnStack, *_typeOnStack.mobileType(), true);
			EnumType const& enumType = dynamic_cast<decltype(enumType)>(_targetType);
			solAssert(enumType.numberOfMembers() > 0, "empty enum should have caused a parser error.");
			m_context << u256(enumType.numberOfMembers() - 1) << Instruction::DUP2 << Instruction::GT;
			m_context.appendConditionalPanic(util::PanicCode::EnumConversionError);
			enumOverflowCheckPending = false;
		}
		else if (targetTypeCategory == Type::Category::FixedPoint)
		{
			solAssert(
				stackTypeCategory == Type::Category::Integer ||
				stackTypeCategory == Type::Category::RationalNumber ||
				stackTypeCategory == Type::Category::FixedPoint,
				"Invalid conversion to FixedMxNType requested."
			);
			//shift all integer bits onto the left side of the fixed type
			FixedPointType const& targetFixedPointType = dynamic_cast<FixedPointType const&>(_targetType);
			if (auto typeOnStack = dynamic_cast<IntegerType const*>(&_typeOnStack))
				if (targetFixedPointType.numBits() > typeOnStack->numBits())
					cleanHigherOrderBits(*typeOnStack);
			solUnimplemented("Not yet implemented - FixedPointType.");
		}
		else
		{
			solAssert(
				targetTypeCategory == Type::Category::Integer ||
				targetTypeCategory == Type::Category::Contract ||
				targetTypeCategory == Type::Category::Address,
				""
			);
			IntegerType addressType(168);  // Solidity++: 168-bit address
			IntegerType const& targetType = targetTypeCategory == Type::Category::Integer
				? dynamic_cast<IntegerType const&>(_targetType) : addressType;
			if (stackTypeCategory == Type::Category::RationalNumber)
			{
				RationalNumberType const& constType = dynamic_cast<RationalNumberType const&>(_typeOnStack);
				// We know that the stack is clean, we only have to clean for a narrowing conversion
				// where cleanup is forced.
				solUnimplementedAssert(!constType.isFractional(), "Not yet implemented - FixedPointType.");
				if (targetType.numBits() < constType.integerType()->numBits() && _cleanupNeeded)
					cleanHigherOrderBits(targetType);
			}
			else
			{
				IntegerType const& typeOnStack = stackTypeCategory == Type::Category::Integer
					? dynamic_cast<IntegerType const&>(_typeOnStack) : addressType;
				// Widening: clean up according to source type width
				// Non-widening and force: clean up according to target type bits
				if (targetType.numBits() > typeOnStack.numBits())
					cleanHigherOrderBits(typeOnStack);
				else if (_cleanupNeeded)
					cleanHigherOrderBits(targetType);
				if (chopSignBitsPending)
				{
					if (targetType.numBits() < 256)
						m_context
							<< ((u256(1) << targetType.numBits()) - 1)
							<< Instruction::AND;
					chopSignBitsPending = false;
				}
			}
		}
		break;
	case Type::Category::StringLiteral:
	{
		auto const& literalType = dynamic_cast<StringLiteralType const&>(_typeOnStack);
		string const& value = literalType.value();
		bytesConstRef data(value);
		if (targetTypeCategory == Type::Category::FixedBytes)
		{
			unsigned const numBytes = dynamic_cast<FixedBytesType const&>(_targetType).numBytes();
			solAssert(data.size() <= 32, "");
			m_context << (u256(h256(data, h256::AlignLeft)) & (~(u256(-1) >> (8 * numBytes))));
		}
		else if (targetTypeCategory == Type::Category::Array)
		{
			auto const& arrayType = dynamic_cast<ArrayType const&>(_targetType);
			solAssert(arrayType.isByteArray(), "");
			size_t storageSize = 32 + ((data.size() + 31) / 32) * 32;
			allocateMemory(storageSize);
			// stack: mempos
			m_context << Instruction::DUP1 << u256(data.size());
			storeInMemoryDynamic(*TypeProvider::uint256());
			// stack: mempos datapos
			storeStringData(data);
		}
		else
			solAssert(
				false,
				"Invalid conversion from string literal to " + _targetType.toString(false) + " requested."
			);
		break;
	}
	case Type::Category::Array:
	{
		solAssert(targetTypeCategory == stackTypeCategory, "");
		auto const& typeOnStack = dynamic_cast<ArrayType const&>(_typeOnStack);
		auto const& targetType = dynamic_cast<ArrayType const&>(_targetType);
		switch (targetType.location())
		{
		case DataLocation::Storage:
			// Other cases are done explicitly in LValue::storeValue, and only possible by assignment.
			solAssert(
				(targetType.isPointer() || (typeOnStack.isByteArray() && targetType.isByteArray())) &&
				typeOnStack.location() == DataLocation::Storage,
				"Invalid conversion to storage type."
			);
			break;
		case DataLocation::Memory:
		{
			// Copy the array to a free position in memory, unless it is already in memory.
			if (typeOnStack.location() != DataLocation::Memory)
			{
				if (
					typeOnStack.dataStoredIn(DataLocation::CallData) &&
					typeOnStack.baseType()->isDynamicallyEncoded()
				)
				{
					solAssert(m_context.useABICoderV2(), "");
					// stack: offset length(optional in case of dynamically sized array)
					solAssert(typeOnStack.sizeOnStack() == (typeOnStack.isDynamicallySized() ? 2 : 1), "");
					if (typeOnStack.isDynamicallySized())
						m_context << Instruction::SWAP1;

					m_context.callYulFunction(
						m_context.utilFunctions().conversionFunction(typeOnStack, targetType),
						typeOnStack.isDynamicallySized() ? 2 : 1,
						1
					);
				}
				else
				{
					// stack: <source ref> (variably sized)
					unsigned stackSize = typeOnStack.sizeOnStack();
					ArrayUtils(m_context).retrieveLength(typeOnStack);

					// allocate memory
					// stack: <source ref> (variably sized) <length>
					m_context << Instruction::DUP1;
					ArrayUtils(m_context).convertLengthToSize(targetType, true);
					// stack: <source ref> (variably sized) <length> <size>
					if (targetType.isDynamicallySized())
						m_context << u256(0x20) << Instruction::ADD;
					allocateMemory();
					// stack: <source ref> (variably sized) <length> <mem start>
					m_context << Instruction::DUP1;
					moveIntoStack(2 + stackSize);
					if (targetType.isDynamicallySized())
					{
						m_context << Instruction::DUP2;
						storeInMemoryDynamic(*TypeProvider::uint256());
					}
					// stack: <mem start> <source ref> (variably sized) <length> <mem data pos>
					if (targetType.baseType()->isValueType())
					{
						copyToStackTop(2 + stackSize, stackSize);
						ArrayUtils(m_context).copyArrayToMemory(typeOnStack);
					}
					else
					{
						m_context << u256(0) << Instruction::SWAP1;
						// stack: <mem start> <source ref> (variably sized) <length> <counter> <mem data pos>
						auto repeat = m_context.newTag();
						m_context << repeat;
						m_context << Instruction::DUP3 << Instruction::DUP3;
						m_context << Instruction::LT << Instruction::ISZERO;
						auto loopEnd = m_context.appendConditionalJump();
						copyToStackTop(3 + stackSize, stackSize);
						copyToStackTop(2 + stackSize, 1);
						ArrayUtils(m_context).accessIndex(typeOnStack, false);
						if (typeOnStack.location() == DataLocation::Storage)
							StorageItem(m_context, *typeOnStack.baseType()).retrieveValue(SourceLocation(), true);
						convertType(*typeOnStack.baseType(), *targetType.baseType(), _cleanupNeeded);
						storeInMemoryDynamic(*targetType.baseType(), true);
						m_context << Instruction::SWAP1 << u256(1) << Instruction::ADD;
						m_context << Instruction::SWAP1;
						m_context.appendJumpTo(repeat);
						m_context << loopEnd;
						m_context << Instruction::POP;
					}
					// stack: <mem start> <source ref> (variably sized) <length> <mem data pos updated>
					popStackSlots(2 + stackSize);
					// Stack: <mem start>
				}
			}
			break;
		}
		case DataLocation::CallData:
			solAssert(
				targetType.isByteArray() &&
				typeOnStack.isByteArray() &&
				typeOnStack.location() == DataLocation::CallData,
				"Invalid conversion to calldata type."
			);
			break;
		}
		break;
	}
	case Type::Category::ArraySlice:
	{
		solAssert(_targetType.category() == Type::Category::Array, "");
		auto& typeOnStack = dynamic_cast<ArraySliceType const&>(_typeOnStack);
		auto const& targetArrayType = dynamic_cast<ArrayType const&>(_targetType);
		solAssert(typeOnStack.arrayType().isImplicitlyConvertibleTo(targetArrayType), "");
		solAssert(
			typeOnStack.arrayType().dataStoredIn(DataLocation::CallData) &&
			typeOnStack.arrayType().isDynamicallySized() &&
			!typeOnStack.arrayType().baseType()->isDynamicallyEncoded(),
			""
		);
		if (!_targetType.dataStoredIn(DataLocation::CallData))
			return convertType(typeOnStack.arrayType(), _targetType);
		break;
	}
	case Type::Category::Struct:
	{
		solAssert(targetTypeCategory == stackTypeCategory, "");
		auto& targetType = dynamic_cast<StructType const&>(_targetType);
		auto& typeOnStack = dynamic_cast<StructType const&>(_typeOnStack);
		solAssert(
			targetType.location() != DataLocation::CallData
		, "");
		switch (targetType.location())
		{
		case DataLocation::Storage:
			// Other cases are done explicitly in LValue::storeValue, and only possible by assignment.
			solAssert(
				targetType.isPointer() &&
				typeOnStack.location() == DataLocation::Storage,
				"Invalid conversion to storage type."
			);
			break;
		case DataLocation::Memory:
			// Copy the array to a free position in memory, unless it is already in memory.
			switch (typeOnStack.location())
			{
			case DataLocation::Storage:
			{
				auto conversionImpl =
					[typeOnStack = &typeOnStack, targetType = &targetType](CompilerContext& _context)
				{
					CompilerUtils utils(_context);
					// stack: <source ref>
					utils.allocateMemory(typeOnStack->memoryDataSize());
					_context << Instruction::SWAP1 << Instruction::DUP2;
					// stack: <memory ptr> <source ref> <memory ptr>
					for (auto const& member: typeOnStack->members(nullptr))
					{
						solAssert(!member.type->containsNestedMapping(), "");
						pair<u256, unsigned> const& offsets = typeOnStack->storageOffsetsOfMember(member.name);
						_context << offsets.first << Instruction::DUP3 << Instruction::ADD;
						_context << u256(offsets.second);
						StorageItem(_context, *member.type).retrieveValue(SourceLocation(), true);
						TypePointer targetMemberType = targetType->memberType(member.name);
						solAssert(!!targetMemberType, "Member not found in target type.");
						utils.convertType(*member.type, *targetMemberType, true);
						utils.storeInMemoryDynamic(*targetMemberType, true);
					}
					_context << Instruction::POP << Instruction::POP;
				};
				if (typeOnStack.recursive())
					m_context.callLowLevelFunction(
						"$convertRecursiveArrayStorageToMemory_" + typeOnStack.identifier() + "_to_" + targetType.identifier(),
						1,
						1,
						conversionImpl
					);
				else
					conversionImpl(m_context);
				break;
			}
			case DataLocation::CallData:
			{
				if (typeOnStack.isDynamicallyEncoded())
				{
					solAssert(m_context.useABICoderV2(), "");
					m_context.callYulFunction(
						m_context.utilFunctions().conversionFunction(typeOnStack, targetType),
						1,
						1
					);
				}
				else
				{
					m_context << Instruction::DUP1;
					m_context << Instruction::CALLDATASIZE;
					m_context << Instruction::SUB;
					abiDecode({&targetType}, false);
				}
				break;
			}
			case DataLocation::Memory:
				// nothing to do
				break;
			}
			break;
		case DataLocation::CallData:
			solAssert(false, "Invalid type conversion target location CallData.");
			break;
		}
		break;
	}
	case Type::Category::Tuple:
	{
		TupleType const& sourceTuple = dynamic_cast<TupleType const&>(_typeOnStack);
		TupleType const& targetTuple = dynamic_cast<TupleType const&>(_targetType);
		solAssert(targetTuple.components().size() == sourceTuple.components().size(), "");
		unsigned depth = sourceTuple.sizeOnStack();
		for (size_t i = 0; i < sourceTuple.components().size(); ++i)
		{
			TypePointer sourceType = sourceTuple.components()[i];
			TypePointer targetType = targetTuple.components()[i];
			if (!sourceType)
			{
				solAssert(!targetType, "");
				continue;
			}
			unsigned sourceSize = sourceType->sizeOnStack();
			unsigned targetSize = targetType ? targetType->sizeOnStack() : 0;
			if (!targetType || *sourceType != *targetType || _cleanupNeeded)
			{
				if (targetType)
				{
					if (sourceSize > 0)
						copyToStackTop(depth, sourceSize);
					convertType(*sourceType, *targetType, _cleanupNeeded);
				}
				if (sourceSize > 0 || targetSize > 0)
				{
					// Move it back into its place.
					for (unsigned j = 0; j < min(sourceSize, targetSize); ++j)
						m_context <<
							swapInstruction(depth + targetSize - sourceSize) <<
							Instruction::POP;
					// Value shrank
					for (unsigned j = targetSize; j < sourceSize; ++j)
					{
						moveToStackTop(depth + targetSize - sourceSize, 1);
						m_context << Instruction::POP;
					}
					// Value grew
					if (targetSize > sourceSize)
						moveIntoStack(depth - sourceSize, targetSize - sourceSize);
				}
			}
			depth -= sourceSize;
		}
		break;
	}
	case Type::Category::Bool:
		solAssert(_targetType == _typeOnStack, "Invalid conversion for bool.");
		if (_cleanupNeeded)
			m_context << Instruction::ISZERO << Instruction::ISZERO;
		break;
	default:
		// we used to allow conversions from function to address
		solAssert(!(stackTypeCategory == Type::Category::Function && targetTypeCategory == Type::Category::Address), "");
		if (stackTypeCategory == Type::Category::Function && targetTypeCategory == Type::Category::Function)
		{
			FunctionType const& typeOnStack = dynamic_cast<FunctionType const&>(_typeOnStack);
			FunctionType const& targetType = dynamic_cast<FunctionType const&>(_targetType);
			solAssert(
				typeOnStack.isImplicitlyConvertibleTo(targetType) &&
				typeOnStack.sizeOnStack() == targetType.sizeOnStack() &&
				(typeOnStack.kind() == FunctionType::Kind::Internal || typeOnStack.kind() == FunctionType::Kind::External) &&
				typeOnStack.kind() == targetType.kind(),
				"Invalid function type conversion requested."
			);
		}
		else
			// All other types should not be convertible to non-equal types.
			solAssert(_typeOnStack == _targetType, "Invalid type conversion requested.");

		if (_cleanupNeeded && _targetType.canBeStored() && _targetType.storageBytes() < 32)
			m_context
				<< ((u256(1) << (8 * _targetType.storageBytes())) - 1)
				<< Instruction::AND;
		break;
	}
    m_context.appendDebugInfo("end of CompilerUtils::convertType()");
	solAssert(!enumOverflowCheckPending, "enum overflow checking missing.");
	solAssert(!chopSignBitsPending, "forgot to chop the sign bits.");
}

void CompilerUtils::pushZeroValue(Type const& _type)
{
	if (auto const* funType = dynamic_cast<FunctionType const*>(&_type))
	{
		if (funType->kind() == FunctionType::Kind::Internal)
		{
			m_context << m_context.lowLevelFunctionTag("$invalidFunction", 0, 0, [](CompilerContext& _context) {
				_context.appendPanic(util::PanicCode::InvalidInternalFunction);
			});
			if (CompilerContext* runCon = m_context.runtimeContext())
			{
				leftShiftNumberOnStack(32);
				m_context << runCon->lowLevelFunctionTag("$invalidFunction", 0, 0, [](CompilerContext& _context) {
					_context.appendPanic(util::PanicCode::InvalidInternalFunction);
				}).toSubAssemblyTag(m_context.runtimeSub());
				m_context << Instruction::OR;
			}
			return;
		}
	}
	auto const* referenceType = dynamic_cast<ReferenceType const*>(&_type);
	if (!referenceType || referenceType->location() == DataLocation::Storage)
	{
		for (size_t i = 0; i < _type.sizeOnStack(); ++i)
			m_context << u256(0);
		return;
	}
	if (referenceType->location() == DataLocation::CallData)
	{
		solAssert(referenceType->sizeOnStack() == 1 || referenceType->sizeOnStack() == 2, "");
		m_context << Instruction::CALLDATASIZE;
		if (referenceType->sizeOnStack() == 2)
			m_context << 0;
		return;
	}

	solAssert(referenceType->location() == DataLocation::Memory, "");
	if (auto arrayType = dynamic_cast<ArrayType const*>(&_type))
		if (arrayType->isDynamicallySized())
		{
			// Push a memory location that is (hopefully) always zero.
			pushZeroPointer();
			return;
		}

	TypePointer type = &_type;
	m_context.callLowLevelFunction(
		"$pushZeroValue_" + referenceType->identifier(),
		0,
		1,
		[type](CompilerContext& _context) {
			CompilerUtils utils(_context);

			utils.allocateMemory(max<u256>(32u, type->memoryDataSize()));
			_context << Instruction::DUP1;

			if (auto structType = dynamic_cast<StructType const*>(type))
				for (auto const& member: structType->members(nullptr))
				{
					utils.pushZeroValue(*member.type);
					utils.storeInMemoryDynamic(*member.type);
				}
			else if (auto arrayType = dynamic_cast<ArrayType const*>(type))
			{
				solAssert(!arrayType->isDynamicallySized(), "");
				if (arrayType->length() > 0)
				{
					_context << arrayType->length() << Instruction::SWAP1;
					// stack: items_to_do memory_pos
					utils.zeroInitialiseMemoryArray(*arrayType);
					// stack: updated_memory_pos
				}
			}
			else
				solAssert(false, "Requested initialisation for unknown type: " + type->toString());

			// remove the updated memory pointer
			_context << Instruction::POP;
		}
	);
}

void CompilerUtils::pushZeroPointer()
{
    m_context.appendDebugInfo("CompilerUtils::pushZeroPointer()");
	m_context << u256(zeroPointer);
}

void CompilerUtils::moveToStackVariable(VariableDeclaration const& _variable)
{
	unsigned const stackPosition = m_context.baseToCurrentStackOffset(m_context.baseStackOffsetOfVariable(_variable));
	unsigned const size = _variable.annotation().type->sizeOnStack();
	solAssert(stackPosition >= size, "Variable size and position mismatch: stackPosition=" + to_string(stackPosition) + ", size=" +
            to_string(size));
	// move variable starting from its top end in the stack
	if (stackPosition - size + 1 > 16)
		BOOST_THROW_EXCEPTION(
			StackTooDeepError() <<
			errinfo_sourceLocation(_variable.location()) <<
			util::errinfo_comment("Stack too deep, try removing local variables.")
		);
	m_context.appendDebugInfo("CompilerUtils::moveToStackVariable(" + _variable.name() + ")");
	for (unsigned i = 0; i < size; ++i)
		m_context << swapInstruction(stackPosition - size + 1) << Instruction::POP;
}

void CompilerUtils::copyToStackTop(unsigned _stackDepth, unsigned _itemSize)
{
	assertThrow(
		_stackDepth <= 16,
		StackTooDeepError,
		"Stack too deep, try removing local variables."
	);
	m_context.appendDebugInfo("CompilerUtils::copyToStackTop(" + to_string(_stackDepth) + ", " + to_string(_itemSize) + ")");
	for (unsigned i = 0; i < _itemSize; ++i)
		m_context << dupInstruction(_stackDepth);
}

void CompilerUtils::moveToStackTop(unsigned _stackDepth, unsigned _itemSize)
{
    m_context.appendDebugInfo("CompilerUtils::moveToStackTop(" + to_string(_stackDepth) + ", " + to_string(_itemSize) + ")");
	moveIntoStack(_itemSize, _stackDepth);
}

void CompilerUtils::moveIntoStack(unsigned _stackDepth, unsigned _itemSize)
{
    m_context.appendDebugInfo("CompilerUtils::moveIntoStack(" + to_string(_stackDepth) + ", " + to_string(_itemSize) + ")");
	if (_stackDepth <= _itemSize)
		for (unsigned i = 0; i < _stackDepth; ++i)
			rotateStackDown(_stackDepth + _itemSize);
	else
		for (unsigned i = 0; i < _itemSize; ++i)
			rotateStackUp(_stackDepth + _itemSize);
}

void CompilerUtils::rotateStackUp(unsigned _items)
{
	assertThrow(
		_items - 1 <= 16,
		StackTooDeepError,
		"Stack too deep, try removing local variables."
	);
	m_context.appendDebugInfo("CompilerUtils::rotateStackUp(" + to_string(_items) + ")");
	for (unsigned i = 1; i < _items; ++i)
		m_context << swapInstruction(_items - i);
    m_context.appendDebugInfo("end of CompilerUtils::rotateStackUp()");
}

void CompilerUtils::rotateStackDown(unsigned _items)
{
	assertThrow(
		_items - 1 <= 16,
		StackTooDeepError,
		"Stack too deep, try removing local variables."
	);
    m_context.appendDebugInfo("CompilerUtils::rotateStackDown(" + to_string(_items) + ")");
	for (unsigned i = 1; i < _items; ++i)
		m_context << swapInstruction(i);
    m_context.appendDebugInfo("end of CompilerUtils::rotateStackDown()");
}

void CompilerUtils::popStackElement(Type const& _type)
{
	popStackSlots(_type.sizeOnStack());
}

void CompilerUtils::popStackSlots(size_t _amount)
{
    m_context.appendDebugInfo("CompilerUtils::popStackSlots(" + to_string(_amount) + ")");
	for (size_t i = 0; i < _amount; ++i)
		m_context << Instruction::POP;
    m_context.appendDebugInfo("end of CompilerUtils::popStackSlots()");
}

void CompilerUtils::popAndJump(unsigned _toHeight, evmasm::AssemblyItem const& _jumpTo)
{
	solAssert(m_context.stackHeight() >= _toHeight, "");
    m_context.appendDebugInfo("CompilerUtils::popAndJump(toHeight=" + to_string(_toHeight) + ", jumpTo=" + _jumpTo.toAssemblyText(m_context.assembly()) + ")");
	unsigned amount = m_context.stackHeight() - _toHeight;
	popStackSlots(amount);
	m_context.appendJumpTo(_jumpTo);
	m_context.adjustStackOffset(static_cast<int>(amount));
    m_context.appendDebugInfo("end of CompilerUtils::popAndJump()");
}

unsigned CompilerUtils::sizeOnStack(vector<Type const*> const& _variableTypes)
{
	unsigned size = 0;
	for (Type const* const& type: _variableTypes)
		size += type->sizeOnStack();
	return size;
}

void CompilerUtils::computeHashStatic()
{
	storeInMemory(0);
	m_context << u256(32) << u256(0) << Instruction::BLAKE2B;
}

void CompilerUtils::copyContractCodeToMemory(ContractDefinition const& contract, bool _creation)
{
    m_context.appendDebugInfo("CompilerUtils::copyContractCodeToMemory()");
	string which = _creation ? "Creation" : "Runtime";
	m_context.callLowLevelFunction(
		"$copyContract" + which + "CodeToMemory_" + contract.type()->identifier(),
		1,
		1,
		[&contract, _creation](CompilerContext& _context)
		{
			// copy the contract's code into memory
			shared_ptr<evmasm::Assembly> assembly =
				_creation ?
				_context.compiledContract(contract) :
				_context.compiledContractRuntime(contract);
			// pushes size
			auto subroutine = _context.addSubroutine(assembly);
			_context << Instruction::DUP1 << subroutine;
			_context << Instruction::DUP4 << Instruction::CODECOPY;
			_context << Instruction::ADD;
		}
	);
}

void CompilerUtils::storeStringData(bytesConstRef _data)
{
	//@todo provide both alternatives to the optimiser
	// stack: mempos
	m_context.appendDebugInfo("CompilerUtils::storeStringData()");
	if (_data.size() <= 32)
	{
		for (unsigned i = 0; i < _data.size(); i += 32)
		{
			m_context << u256(h256(_data.cropped(i), h256::AlignLeft));
			storeInMemoryDynamic(*TypeProvider::uint256());
		}
		m_context << Instruction::POP;
	}
	else
	{
		// stack: mempos mempos_data
		m_context.appendData(_data.toBytes());
		m_context << u256(_data.size()) << Instruction::SWAP2;
		m_context << Instruction::CODECOPY;
	}
}

unsigned CompilerUtils::loadFromMemoryHelper(Type const& _type, bool _fromCalldata, bool _padToWords)
{
	solAssert(_type.isValueType(), "");
	m_context.appendDebugInfo("CompilerUtils::loadFromMemoryHelper()");
	unsigned numBytes = _type.calldataEncodedSize(_padToWords);
	bool isExternalFunctionType = false;
	if (auto const* funType = dynamic_cast<FunctionType const*>(&_type))
		if (funType->kind() == FunctionType::Kind::External)
			isExternalFunctionType = true;
	if (numBytes == 0)
	{
		m_context << Instruction::POP << u256(0);
		return numBytes;
	}
	solAssert(numBytes <= 32, "Static memory load of more than 32 bytes requested.");
	m_context << (_fromCalldata ? Instruction::CALLDATALOAD : Instruction::MLOAD);
	bool cleanupNeeded = true;
	if (isExternalFunctionType)
		splitExternalFunctionType(true);
	else if (numBytes != 32)
	{
		bool leftAligned = _type.category() == Type::Category::FixedBytes;
		// add leading or trailing zeros by dividing/multiplying depending on alignment
		unsigned shiftFactor = (32 - numBytes) * 8;
		rightShiftNumberOnStack(shiftFactor);
		if (leftAligned)
		{
			leftShiftNumberOnStack(shiftFactor);
			cleanupNeeded = false;
		}
		else if (IntegerType const* intType = dynamic_cast<IntegerType const*>(&_type))
			if (!intType->isSigned())
				cleanupNeeded = false;
	}
	if (_fromCalldata)
		convertType(_type, _type, cleanupNeeded, false, true);

	return numBytes;
}

void CompilerUtils::cleanHigherOrderBits(IntegerType const& _typeOnStack)
{
	if (_typeOnStack.numBits() == 256)
		return;
	else if (_typeOnStack.isSigned())
		m_context << u256(_typeOnStack.numBits() / 8 - 1) << Instruction::SIGNEXTEND;
	else
		m_context << ((u256(1) << _typeOnStack.numBits()) - 1) << Instruction::AND;
}

void CompilerUtils::leftShiftNumberOnStack(unsigned _bits)
{
	solAssert(_bits < 256, "");
	m_context.appendDebugInfo("CompilerUtils::leftShiftNumberOnStack(bits=" + to_string(_bits) + ")");
	if (m_context.evmVersion().hasBitwiseShifting())
		m_context << _bits << Instruction::SHL;
	else
		m_context << (u256(1) << _bits) << Instruction::MUL;
    m_context.appendDebugInfo("end of CompilerUtils::leftShiftNumberOnStack()");
}

void CompilerUtils::rightShiftNumberOnStack(unsigned _bits)
{
	solAssert(_bits < 256, "");
    m_context.appendDebugInfo("CompilerUtils::rightShiftNumberOnStack(bits=" + to_string(_bits) + ")");
	// NOTE: If we add signed right shift, SAR rounds differently than SDIV
	if (m_context.evmVersion().hasBitwiseShifting())
		m_context << _bits << Instruction::SHR;
	else
		m_context << (u256(1) << _bits) << Instruction::SWAP1 << Instruction::DIV;
    m_context.appendDebugInfo("end of CompilerUtils::rightShiftNumberOnStack()");
}

unsigned CompilerUtils::prepareMemoryStore(Type const& _type, bool _padToWords, bool _cleanup)
{
	solAssert(
		_type.sizeOnStack() == 1,
		"Memory store of types with stack size != 1 not allowed (Type: " + _type.toString(true) + ")."
	);

	solAssert(!_type.isDynamicallyEncoded(), "");

	unsigned numBytes = _type.calldataEncodedSize(_padToWords);

	m_context.appendDebugInfo("CompilerUtils::prepareMemoryStore(type=" + _type.toString(false) + ", ...)");

	solAssert(
		numBytes > 0,
		"Memory store of 0 bytes requested (Type: " + _type.toString(true) + ")."
	);

	solAssert(
		numBytes <= 32,
		"Memory store of more than 32 bytes requested (Type: " + _type.toString(true) + ")."
	);

	bool leftAligned = _type.category() == Type::Category::FixedBytes;

	if (_cleanup)
		convertType(_type, _type, true);

	if (numBytes != 32 && !leftAligned && !_padToWords)
		// shift the value accordingly before storing
		leftShiftNumberOnStack((32 - numBytes) * 8);

    m_context.appendDebugInfo("end of CompilerUtils::prepareMemoryStore()");
	return numBytes;
}

unsigned CompilerUtils::sizeOnCalldata(const TypePointers _types)
{
    unsigned size = 0;
    for(auto t : _types)
        size += t->calldataHeadSize();
    return size;
}
