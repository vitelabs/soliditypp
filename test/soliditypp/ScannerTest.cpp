#define BOOST_TEST_INCLUDEED
#include <boost/test/unit_test.hpp>

#include <liblangutil/Scanner.h>

using namespace std;
using namespace solidity::langutil;

namespace soliditypp::langutil::test::soliditypp
{

BOOST_AUTO_TEST_SUITE(SolidityppScannerTest)

BOOST_AUTO_TEST_CASE(soliditypp_keywords)
{
	string keywords = "message onMessage getter";
	Scanner scanner(CharStream(keywords, ""));
	BOOST_CHECK_EQUAL(scanner.currentToken(), Token::Message);
	BOOST_CHECK_EQUAL(scanner.next(), Token::OnMessage);
	BOOST_CHECK_EQUAL(scanner.next(), Token::Getter);
	BOOST_CHECK_EQUAL(scanner.next(), Token::EOS);
}

BOOST_AUTO_TEST_CASE(vite_subdenominations)
{
	Scanner scanner(CharStream("vite attov", ""));
	BOOST_CHECK_EQUAL(scanner.currentToken(), Token::SubVite);
	BOOST_CHECK_EQUAL(scanner.next(), Token::SubAttov);
	BOOST_CHECK_EQUAL(scanner.next(), Token::EOS);
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespaces
