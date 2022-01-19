#include <libsolidity/ast/Types.h>
#include <libsolidity/ast/TypeProvider.h>

#include <liblangutil/Exceptions.h>
#include <libsolidity/ast/AST.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace solidity::langutil;

namespace solidity::frontend::test
{

BOOST_AUTO_TEST_SUITE(AST)

BOOST_AUTO_TEST_CASE(literal_looks_like_vite_address)
{
	std::shared_ptr<langutil::CharStream> source = std::make_shared<langutil::CharStream>("", "");
	SourceLocation sourceLocation = SourceLocation{0, 0, source};
	// valid vite address
	BOOST_CHECK(Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("vite_010203040506070809080706050403020102030412227c8b71")).looksLikeViteAddress());
	// invalid: too short
	BOOST_CHECK(Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("vite_0102030405060708090807060504030201020304122")).looksLikeViteAddress());
	// invalid: empty hex
	BOOST_CHECK(Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("vite_")).looksLikeViteAddress());
	// invalid: too long
	BOOST_CHECK(Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("vite_010203040506070809080706050403020102030412227c8b71000000")).looksLikeViteAddress());
	// invalid: bad prefix
	BOOST_CHECK(!Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("0x010203040506070809080706050403020102030412227c8b71")).looksLikeViteAddress());
	BOOST_CHECK(!Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("010203040506070809080706050403020102030412227c8b71")).looksLikeViteAddress());
}

BOOST_AUTO_TEST_CASE(literal_looks_like_vite_token_id)
{
	std::shared_ptr<langutil::CharStream> source = std::make_shared<langutil::CharStream>("", "");
	SourceLocation sourceLocation = SourceLocation{0, 0, source};
	// valid vite token id
	BOOST_CHECK(Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("tti_5649544520544f4b454e6e40")).looksLikeViteTokenId());
	// invalid: too short
	BOOST_CHECK(Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("tti_5649544520544f4b454e")).looksLikeViteTokenId());
	// invalid: empty hex
	BOOST_CHECK(Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("tti_")).looksLikeViteTokenId());
	// invalid: too long
	BOOST_CHECK(Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("tti_5649544520544f4b454e6e400000")).looksLikeViteTokenId());
	// invalid: bad prefix
	BOOST_CHECK(!Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("0x5649544520544f4b454e6e40")).looksLikeViteTokenId());
	BOOST_CHECK(!Literal(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("5649544520544f4b454e6e40")).looksLikeViteTokenId());
}

BOOST_AUTO_TEST_CASE(literal_vite_address_hex)
{
	std::shared_ptr<langutil::CharStream> source = std::make_shared<langutil::CharStream>("", "");
	SourceLocation sourceLocation = SourceLocation{0, 0, source};
	Literal address1(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("vite_010203040506070809080706050403020102030412227c8b71"));
	// user address
	BOOST_CHECK_EQUAL(address1.getViteAddressHex(), "0x010203040506070809080706050403020102030400");

	// contract address
	Literal address2(2, sourceLocation, Token::StringLiteral, make_shared<ASTString>("vite_8cf2663cc949442db2d3f78f372621733292d1fb0b846f1651"));
	BOOST_CHECK_EQUAL(address2.getViteAddressHex(), "0x8cf2663cc949442db2d3f78f372621733292d1fb01");
}

BOOST_AUTO_TEST_CASE(literal_vite_tokenid_hex)
{
	std::shared_ptr<langutil::CharStream> source = std::make_shared<langutil::CharStream>("", "");
	SourceLocation sourceLocation = SourceLocation{0, 0, source};
	Literal token(1, sourceLocation, Token::StringLiteral, make_shared<ASTString>("tti_5649544520544f4b454e6e40"));

	BOOST_CHECK_EQUAL(token.getViteTokenIdHex(), "0x5649544520544f4b454e");
}

BOOST_AUTO_TEST_SUITE_END()

}
