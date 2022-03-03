#define BOOST_TEST_INCLUDEED
#include <boost/test/unit_test.hpp>

#include <liblangutil/Scanner.h>
#include <libsolidity/parsing/Parser.h>
#include <liblangutil/ErrorReporter.h>

using namespace std;
using namespace solidity::langutil;

namespace solidity::frontend::test::soliditypp
{
    ASTPointer<ContractDefinition> parseText(std::string const& _source, ErrorList& _errors, bool errorRecovery = false)
    {
        ErrorReporter errorReporter(_errors);
        ASTPointer<SourceUnit> sourceUnit = Parser(
            errorReporter,
            langutil::EVMVersion(),
            errorRecovery
        ).parse(std::make_shared<Scanner>(CharStream(_source, "")));
        if (!sourceUnit)
            return ASTPointer<ContractDefinition>();
        for (ASTPointer<ASTNode> const& node: sourceUnit->nodes())
            if (ASTPointer<ContractDefinition> contract = dynamic_pointer_cast<ContractDefinition>(node))
                return contract;
        BOOST_FAIL("No contract found in source.");
        return ASTPointer<ContractDefinition>();
    }

    bool successParse(std::string const& _source)
    {
        ErrorList errors;
        try
        {
            auto sourceUnit = parseText(_source, errors);
            if (!sourceUnit)
                return false;
        }
        catch (FatalError const& /*_exception*/)
        {
            if (Error::containsErrorOfType(errors, Error::Type::ParserError))
                return false;
        }
        if (Error::containsErrorOfType(errors, Error::Type::ParserError))
            return false;

        BOOST_CHECK(Error::containsOnlyWarnings(errors));
        return true;
    }

    BOOST_AUTO_TEST_SUITE(SolidityppParserTest)

    BOOST_AUTO_TEST_CASE(soliditypp_await)
    {
        char const* text = R"(
            contract B {
                A a;
                function test() returns(uint256) {
                    return await a.f();
                }
            }
        )";
        BOOST_CHECK(successParse(text));
    }

    BOOST_AUTO_TEST_CASE(soliditypp_address_literal)
    {
        char const* text = R"(
            contract Test {
                address a = "vite_010203040506070809080706050403020102030412227c8b71";
            }
        )";
        BOOST_CHECK(successParse(text));
    }

    BOOST_AUTO_TEST_CASE(soliditypp_tokenid_literal)
    {
        char const* text = R"(
            contract Test {
                vitetoken token = "tti_5649544520544f4b454e6e40";
            }
        )";
        BOOST_CHECK(successParse(text));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
