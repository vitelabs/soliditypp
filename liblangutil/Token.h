// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ Tokens.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#pragma once

#include <libsolutil/Common.h>
#include <liblangutil/Exceptions.h>
#include <liblangutil/UndefMacros.h>

#include <iosfwd>
#include <string>
#include <tuple>

namespace solidity::langutil
{

// TOKEN_LIST takes a list of 3 macros M, all of which satisfy the
// same signature M(name, string, precedence), where name is the
// symbolic token name, string is the corresponding syntactic symbol
// (or nullptr, for literals), and precedence is the precedence (or 0).
// The parameters are invoked for token categories as follows:
//
//   T: Non-keyword tokens
//   K: Keyword tokens

// IGNORE_TOKEN is a convenience macro that can be supplied as
// an argument (at any position) for a TOKEN_LIST call. It does
// nothing with tokens belonging to the respective category.

#define IGNORE_TOKEN(name, string, precedence)

#define TOKEN_LIST(T, K)												\
	/* End of source indicator. */										\
	T(EOS, "EOS", 0)													\
																		\
	/* Punctuators (ECMA-262, section 7.7, page 15). */				\
	T(LParen, "(", 0)                                                   \
	T(RParen, ")", 0)                                                   \
	T(LBrack, "[", 0)                                                   \
	T(RBrack, "]", 0)                                                   \
	T(LBrace, "{", 0)                                                   \
	T(RBrace, "}", 0)                                                   \
	T(Colon, ":", 0)                                                    \
	T(Semicolon, ";", 0)                                                \
	T(Period, ".", 0)                                                   \
	T(Conditional, "?", 3)                                              \
	T(DoubleArrow, "=>", 0)                                             \
	T(RightArrow, "->", 0)                                              \
	\
	/* Assignment operators. */										\
	/* IsAssignmentOp() relies on this block of enum values being */	\
	/* contiguous and sorted in the same order!*/						\
	T(Assign, "=", 2)                                                   \
	/* The following have to be in exactly the same order as the simple binary operators*/ \
	T(AssignBitOr, "|=", 2)                                           \
	T(AssignBitXor, "^=", 2)                                          \
	T(AssignBitAnd, "&=", 2)                                          \
	T(AssignShl, "<<=", 2)                                            \
	T(AssignSar, ">>=", 2)                                            \
	T(AssignShr, ">>>=", 2)                                           \
	T(AssignAdd, "+=", 2)                                             \
	T(AssignSub, "-=", 2)                                             \
	T(AssignMul, "*=", 2)                                             \
	T(AssignDiv, "/=", 2)                                             \
	T(AssignMod, "%=", 2)                                             \
	\
	/* Binary operators sorted by precedence. */                       \
	/* IsBinaryOp() relies on this block of enum values */             \
	/* being contiguous and sorted in the same order! */               \
	T(Comma, ",", 1)                                                   \
	T(Or, "||", 4)                                                     \
	T(And, "&&", 5)                                                    \
	T(BitOr, "|", 8)                                                   \
	T(BitXor, "^", 9)                                                  \
	T(BitAnd, "&", 10)                                                 \
	T(SHL, "<<", 11)                                                   \
	T(SAR, ">>", 11)                                                   \
	T(SHR, ">>>", 11)                                                  \
	T(Add, "+", 12)                                                    \
	T(Sub, "-", 12)                                                    \
	T(Mul, "*", 13)                                                    \
	T(Div, "/", 13)                                                    \
	T(Mod, "%", 13)                                                    \
	T(Exp, "**", 14)                                                   \
	\
	/* Compare operators sorted by precedence. */                      \
	/* IsCompareOp() relies on this block of enum values */            \
	/* being contiguous and sorted in the same order! */               \
	T(Equal, "==", 6)                                                  \
	T(NotEqual, "!=", 6)                                               \
	T(LessThan, "<", 7)                                                \
	T(GreaterThan, ">", 7)                                             \
	T(LessThanOrEqual, "<=", 7)                                        \
	T(GreaterThanOrEqual, ">=", 7)                                     \
	\
	/* Unary operators. */                                             \
	/* IsUnaryOp() relies on this block of enum values */              \
	/* being contiguous and sorted in the same order! */               \
	T(Not, "!", 0)                                                     \
	T(BitNot, "~", 0)                                                  \
	T(Inc, "++", 0)                                                    \
	T(Dec, "--", 0)                                                    \
	K(Delete, "delete", 0)                                             \
	\
	/* Inline Assembly Operators */                                    \
	T(AssemblyAssign, ":=", 2)                                         \
	/* Keywords */                                                     \
	K(Abstract, "abstract", 0)                                         \
	K(Anonymous, "anonymous", 0)                                       \
	K(As, "as", 0)                                                     \
	K(Assembly, "assembly", 0)                                         \
	K(Break, "break", 0)                                               \
	K(Catch, "catch", 0)                                               \
	K(Constant, "constant", 0)                                         \
	K(Constructor, "constructor", 0)                                   \
	K(Continue, "continue", 0)                                         \
	K(Contract, "contract", 0)                                         \
	K(Do, "do", 0)                                                     \
	K(Else, "else", 0)                                                 \
	K(Enum, "enum", 0)                                                 \
	K(Emit, "emit", 0)                                                 \
	/* Solidity++: send keyword for send statement */				   \
	K(Send, "send", 0)												   \
	\
	K(Event, "event", 0)                                               \
    \
	K(External, "external", 0)                                         \
	K(Fallback, "fallback", 0)                                         \
	K(For, "for", 0)                                                   \
	K(Function, "function", 0)                                         \
	K(Hex, "hex", 0)                                                   \
	K(If, "if", 0)                                                     \
	K(Indexed, "indexed", 0)                                           \
	K(Interface, "interface", 0)                                       \
	K(Internal, "internal", 0)                                         \
	K(Immutable, "immutable", 0)                                       \
	K(Import, "import", 0)                                             \
	K(Is, "is", 0)                                                     \
	K(Library, "library", 0)                                           \
	K(Mapping, "mapping", 0)                                           \
	K(Memory, "memory", 0)                                             \
	K(Modifier, "modifier", 0)                                         \
	K(New, "new", 0)                                                   \
	K(Override, "override", 0)                                         \
	K(Payable, "payable", 0)                                           \
	K(Public, "public", 0)                                             \
	K(Pragma, "pragma", 0)                                             \
	K(Private, "private", 0)                                           \
	K(Pure, "pure", 0)                                                 \
	K(Receive, "receive", 0)                                           \
	K(Return, "return", 0)                                             \
	K(Returns, "returns", 0)                                           \
	K(Storage, "storage", 0)                                           \
	K(CallData, "calldata", 0)                                         \
	K(Struct, "struct", 0)                                             \
	K(Throw, "throw", 0)                                               \
	K(Try, "try", 0)                                                   \
	K(Type, "type", 0)                                                 \
	K(Unchecked, "unchecked", 0)                                       \
	K(Unicode, "unicode", 0)                                           \
	K(Using, "using", 0)                                               \
	K(View, "view", 0)                                                 \
	K(Virtual, "virtual", 0)                                           \
	K(While, "while", 0)                                               \
	\
	/* Ether subdenominations */                                       \
	K(SubWei, "wei", 0)                                                \
	K(SubGwei, "gwei", 0)                                              \
	K(SubEther, "ether", 0)                                            \
	K(SubSecond, "seconds", 0)                                         \
	K(SubMinute, "minutes", 0)                                         \
	K(SubHour, "hours", 0)                                             \
	K(SubDay, "days", 0)                                               \
	K(SubWeek, "weeks", 0)                                             \
	K(SubYear, "years", 0)                                             \
	\
	/* Solidity++: Vite subdenominations */                            \
	K(SubAttov, "attov", 0)											   \
	K(SubVite, "vite", 0)                                              \
	\
	/* type keywords*/                                                 \
	K(Int, "int", 0)                                                   \
	K(UInt, "uint", 0)                                                 \
	K(Bytes, "bytes", 0)                                               \
	K(String, "string", 0)                                             \
	K(Address, "address", 0)                                           \
	K(Bool, "bool", 0)                                                 \
	K(Fixed, "fixed", 0)                                               \
	K(UFixed, "ufixed", 0)                                             \
	T(IntM, "intM", 0)                                                 \
	T(UIntM, "uintM", 0)                                               \
	T(BytesM, "bytesM", 0)                                             \
	T(FixedMxN, "fixedMxN", 0)                                         \
	T(UFixedMxN, "ufixedMxN", 0)                                       \
	K(TokenId, "vitetoken", 0) /* Solidity++: Vite token id */           \
	T(TypesEnd, nullptr, 0) /* used as type enum end marker */         \
	\
	/* Literals */                                                     \
	K(TrueLiteral, "true", 0)                                          \
	K(FalseLiteral, "false", 0)                                        \
	T(Number, nullptr, 0)                                              \
	T(StringLiteral, nullptr, 0)                                       \
	T(UnicodeStringLiteral, nullptr, 0)                                \
	T(HexStringLiteral, nullptr, 0)                                    \
	T(CommentLiteral, nullptr, 0)                                      \
	\
	/* Identifiers (not keywords or future reserved words). */         \
	T(Identifier, nullptr, 0)                                          \
	\
	/* Keywords reserved for future use. */                            \
	K(After, "after", 0)                                               \
	K(Alias, "alias", 0)                                               \
	K(Apply, "apply", 0)                                               \
	K(Auto, "auto", 0)                                                 \
	K(Byte, "byte", 0)                                                 \
	K(Case, "case", 0)                                                 \
	K(CopyOf, "copyof", 0)                                             \
	K(Default, "default", 0)                                           \
	K(Define, "define", 0)                                             \
	K(Final, "final", 0)                                               \
	K(Implements, "implements", 0)                                     \
	K(In, "in", 0)                                                     \
	K(Inline, "inline", 0)                                             \
	K(Let, "let", 0)                                                   \
	K(Macro, "macro", 0)                                               \
	K(Match, "match", 0)                                               \
	K(Mutable, "mutable", 0)                                           \
	K(NullLiteral, "null", 0)                                          \
	K(Of, "of", 0)                                                     \
	K(Partial, "partial", 0)                                           \
	K(Promise, "promise", 0)                                           \
	K(Reference, "reference", 0)                                       \
	K(Relocatable, "relocatable", 0)                                   \
	K(Sealed, "sealed", 0)                                             \
	K(Sizeof, "sizeof", 0)                                             \
	K(Static, "static", 0)                                             \
	K(Supports, "supports", 0)                                         \
	K(Switch, "switch", 0)                                             \
	K(Typedef, "typedef", 0)                                           \
	K(TypeOf, "typeof", 0)                                             \
	K(Var, "var", 0)                                                   \
	\
	/* Solidity++ keywords */                            			   \
    K(Async, "async", 0)                                               \
	K(Await, "await", 0)                                               \
	\
	/* Yul-specific tokens, but not keywords. */                       \
	T(Leave, "leave", 0)                                               \
	\
	/* Illegal token - not able to scan. */                            \
	T(Illegal, "ILLEGAL", 0)                                           \
	\
	/* Scanner-internal use only. */                                   \
	T(Whitespace, nullptr, 0)

// All token values.
// attention! msvc issue:
// http://stackoverflow.com/questions/9567868/compile-errors-after-adding-v8-to-my-project-c2143-c2059
// @todo: avoid TOKEN_LIST macro
enum class Token : unsigned int {
#define T(name, string, precedence) name,
	TOKEN_LIST(T, T)
	NUM_TOKENS
#undef T
};

namespace TokenTraits
{
	constexpr size_t count() { return static_cast<size_t>(Token::NUM_TOKENS); }

	// Predicates
	constexpr bool isElementaryTypeName(Token tok) { return Token::Int <= tok && tok < Token::TypesEnd; }
	constexpr bool isAssignmentOp(Token tok) { return Token::Assign <= tok && tok <= Token::AssignMod; }
	constexpr bool isBinaryOp(Token op) { return Token::Comma <= op && op <= Token::Exp; }
	constexpr bool isCommutativeOp(Token op) { return op == Token::BitOr || op == Token::BitXor || op == Token::BitAnd ||
		op == Token::Add || op == Token::Mul || op == Token::Equal || op == Token::NotEqual; }
	constexpr bool isArithmeticOp(Token op) { return Token::Add <= op && op <= Token::Exp; }
	constexpr bool isCompareOp(Token op) { return Token::Equal <= op && op <= Token::GreaterThanOrEqual; }

	constexpr bool isBitOp(Token op) { return (Token::BitOr <= op && op <= Token::BitAnd) || op == Token::BitNot; }
	constexpr bool isBooleanOp(Token op) { return (Token::Or <= op && op <= Token::And) || op == Token::Not; }
	constexpr bool isUnaryOp(Token op) { return (Token::Not <= op && op <= Token::Delete) || op == Token::Add || op == Token::Sub; }
	constexpr bool isCountOp(Token op) { return op == Token::Inc || op == Token::Dec; }
	constexpr bool isShiftOp(Token op) { return (Token::SHL <= op) && (op <= Token::SHR); }
	constexpr bool isVariableVisibilitySpecifier(Token op) { return op == Token::Public || op == Token::Private || op == Token::Internal; }
	constexpr bool isVisibilitySpecifier(Token op) { return isVariableVisibilitySpecifier(op) || op == Token::External; }
	constexpr bool isLocationSpecifier(Token op) { return op == Token::Memory || op == Token::Storage || op == Token::CallData; }

	constexpr bool isStateMutabilitySpecifier(Token op)
	{
		return op == Token::Pure || op == Token::View || op == Token::Payable;
	}

	// Solidity++: execution behavior specifier: sync / async
    constexpr bool isExecutionBehaviorSpecifier(Token op) { return op == Token::Async; }

	constexpr bool isEtherSubdenomination(Token op) { return op >= Token::SubWei && op <= Token::SubEther; }
	
	// Solidity++: Vite Subdenomination
	constexpr bool isViteSubdenomination(Token op) { return op >= Token::SubAttov && op <= Token::SubVite; }

	constexpr bool isTimeSubdenomination(Token op) { return op == Token::SubSecond || op == Token::SubMinute || op == Token::SubHour || op == Token::SubDay || op == Token::SubWeek || op == Token::SubYear; }
	constexpr bool isReservedKeyword(Token op) { return (Token::After <= op && op <= Token::Var); }

	constexpr bool isYulKeyword(Token tok)
	{
		return tok == Token::Function || tok == Token::Let || tok == Token::If || tok == Token::Switch || tok == Token::Case ||
			tok == Token::Default || tok == Token::For || tok == Token::Break || tok == Token::Continue || tok == Token::Leave ||
			tok == Token::TrueLiteral || tok == Token::FalseLiteral || tok == Token::HexStringLiteral || tok == Token::Hex;
	}

	bool isYulKeyword(std::string const& _literal);

	inline Token AssignmentToBinaryOp(Token op)
	{
		solAssert(isAssignmentOp(op) && op != Token::Assign, "");
		return static_cast<Token>(static_cast<int>(op) + (static_cast<int>(Token::BitOr) - static_cast<int>(Token::AssignBitOr)));
	}

	// @returns the precedence > 0 for binary and compare
	// operators; returns 0 otherwise.
	constexpr int precedence(Token tok)
	{
		int8_t constexpr precs[TokenTraits::count()] =
		{
			#define T(name, string, precedence) precedence,
			TOKEN_LIST(T, T)
			#undef T
		};
		return precs[static_cast<size_t>(tok)];
	}

	constexpr bool hasExpHighestPrecedence()
	{
		constexpr int expPrecedence = TokenTraits::precedence(Token::Exp);
		static_assert(expPrecedence == 14, "Exp precedence changed.");

		#define T(name, string, precedence) ((Token::name == Token::Exp) || precedence < expPrecedence) &&
		return
			TOKEN_LIST(T, T)
			true;
		#undef T
	}

	std::tuple<Token, unsigned int, unsigned int> fromIdentifierOrKeyword(std::string const& _literal);

	// @returns a string corresponding to the C++ token name
	// (e.g. "LT" for the token LT).
	char const* name(Token tok);

	// @returns a string corresponding to the JS token string
	// (.e., "<" for the token LT) or nullptr if the token doesn't
	// have a (unique) string (e.g. an IDENTIFIER).
	char const* toString(Token tok);

	std::string friendlyName(Token tok);
}

inline std::ostream& operator<<(std::ostream& os, Token token)
{
	os << TokenTraits::friendlyName(token);
	return os;
}

class ElementaryTypeNameToken
{
public:
	ElementaryTypeNameToken(Token _token, unsigned const& _firstNumber, unsigned const& _secondNumber)
	{
		assertDetails(_token, _firstNumber, _secondNumber);
	}

	unsigned int firstNumber() const { return m_firstNumber; }
	unsigned int secondNumber() const { return m_secondNumber; }
	Token token() const { return m_token; }

	///if tokValue is set to true, then returns the actual token type name, otherwise, returns full type
	std::string toString(bool const& tokenValue = false) const
	{
		std::string name = TokenTraits::toString(m_token);
		if (tokenValue || (firstNumber() == 0 && secondNumber() == 0))
			return name;
		solAssert(name.size() >= 3, "Token name size should be greater than 3. Should not reach here.");
		if (m_token == Token::FixedMxN || m_token == Token::UFixedMxN)
			return name.substr(0, name.size() - 3) + std::to_string(m_firstNumber) + "x" + std::to_string(m_secondNumber);
		else
			return name.substr(0, name.size() - 1) + std::to_string(m_firstNumber);
	}

private:
	Token m_token;
	unsigned int m_firstNumber;
	unsigned int m_secondNumber;
	/// throws if type is not properly sized
	void assertDetails(Token _baseType, unsigned const& _first, unsigned const& _second);
};

}
