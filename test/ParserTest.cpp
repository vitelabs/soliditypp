#define BOOST_TEST_MODULE ParserTest

#include <liblangutil/Scanner.h>
#include <libsolidity/parsing/Parser.h>
#include <liblangutil/ErrorReporter.h>

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace solidity::langutil;

namespace solidity::frontend::test
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

    BOOST_AUTO_TEST_SUITE(ParserTest)

    BOOST_AUTO_TEST_CASE(reserved_keywords)
    {
        BOOST_CHECK(!TokenTraits::isReservedKeyword(Token::Identifier));
        BOOST_CHECK(TokenTraits::isReservedKeyword(Token::After));
        BOOST_CHECK(!TokenTraits::isReservedKeyword(Token::Unchecked));
        BOOST_CHECK(TokenTraits::isReservedKeyword(Token::Var));
        BOOST_CHECK(TokenTraits::isReservedKeyword(Token::Reference));
        BOOST_CHECK(!TokenTraits::isReservedKeyword(Token::Illegal));
    }

    BOOST_AUTO_TEST_CASE(variable_definition)
    {
        char const* text = R"(
            contract test {
                function fun(uint256 a) {
                    uint b;
                    uint256 c;
                    mapping(address=>bytes32) d;
                    customtype varname;
                }
            }
        )";
        BOOST_CHECK(successParse(text));
    }

    BOOST_AUTO_TEST_CASE(soliditypp_message_definition)
    {
        char const* text = R"(
            contract TestMessage {
                message transfer(address indexed addr,uint256 amount);
            }
        )";
        BOOST_CHECK(successParse(text));
    }

    BOOST_AUTO_TEST_CASE(soliditypp_onMessage_definition)
    {
        char const* text = R"(
            contract TestOnMessage {
                event transfer(address indexed addr,uint256 amount);
                onMessage SayHello(address addr) payable {
                    addr.transfer(msg.tokenid ,msg.amount);
                    emit transfer(addr, msg.amount);
                }
            }
        )";
        BOOST_CHECK(successParse(text));
    }

    BOOST_AUTO_TEST_CASE(soliditypp_getter_definition)
    {
        char const* text = R"(
            contract TestGetter {
                uint magic = 0;
                getter getMagic() returns(uint256) {
                    return magic;
                }
            }
        )";
        BOOST_CHECK(successParse(text));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
