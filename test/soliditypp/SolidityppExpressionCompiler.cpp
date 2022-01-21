#include <string>

#include <liblangutil/Scanner.h>
#include <libsolidity/parsing/Parser.h>
#include <libsolidity/analysis/NameAndTypeResolver.h>
#include <libsolidity/analysis/Scoper.h>
#include <libsolidity/analysis/SolidityppSyntaxChecker.h>
#include <libsolidity/analysis/DeclarationTypeChecker.h>
#include <libsolidity/codegen/CompilerContext.h>
#include <libsolidity/codegen/ExpressionCompiler.h>
#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/TypeProvider.h>
#include <libsolidity/analysis/SolidityppTypeChecker.h>
#include <liblangutil/ErrorReporter.h>
#include <libevmasm/LinkerObject.h>
#include <test/Common.h>

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace solidity::evmasm;
using namespace solidity::langutil;

namespace solidity::frontend::test
{

namespace
{

/// Helper class that extracts the first expression in an AST.
class FirstExpressionExtractor: private ASTVisitor
{
public:
	FirstExpressionExtractor(ASTNode& _node): m_expression(nullptr) { _node.accept(*this); }
	Expression* expression() const { return m_expression; }
private:
	bool visit(Assignment& _expression) override { return checkExpression(_expression); }
	bool visit(UnaryOperation& _expression) override { return checkExpression(_expression); }
	bool visit(BinaryOperation& _expression) override { return checkExpression(_expression); }
	bool visit(FunctionCall& _expression) override { return checkExpression(_expression); }
	bool visit(MemberAccess& _expression) override { return checkExpression(_expression); }
	bool visit(IndexAccess& _expression) override { return checkExpression(_expression); }
	bool visit(Identifier& _expression) override { return checkExpression(_expression); }
	bool visit(ElementaryTypeNameExpression& _expression) override { return checkExpression(_expression); }
	bool visit(Literal& _expression) override { return checkExpression(_expression); }
	bool checkExpression(Expression& _expression)
	{
		if (m_expression == nullptr)
			m_expression = &_expression;
		return false;
	}
private:
	Expression* m_expression;
};

Declaration const& resolveDeclaration(
	SourceUnit const& _sourceUnit,
	vector<string> const& _namespacedName,
	NameAndTypeResolver const& _resolver
)
{
	ASTNode const* scope = &_sourceUnit;
	// bracers are required, cause msvc couldn't handle this macro in for statement
	for (string const& namePart: _namespacedName)
	{
		auto declarations = _resolver.resolveName(namePart, scope);
		BOOST_REQUIRE(!declarations.empty());
		BOOST_REQUIRE(scope = *declarations.begin());
	}
	BOOST_REQUIRE(scope);
	return dynamic_cast<Declaration const&>(*scope);
}

bytes compileFirstExpression(
	string const& _sourceCode,
	vector<vector<string>> _functions = {},
	vector<vector<string>> _localVariables = {}
)
{
	string sourceCode = "pragma solidity >=0.0; // SPDX-License-Identifier: GPL-3\n" + _sourceCode;

	ASTPointer<SourceUnit> sourceUnit;
	try
	{
		ErrorList errors;
		ErrorReporter errorReporter(errors);
		sourceUnit = Parser(errorReporter, solidity::test::CommonOptions::get().evmVersion()).parse(
			make_shared<Scanner>(CharStream(sourceCode, ""))
		);
		if (!errors.empty()) {
		    for(auto error : errors) {
                cout << "Error: " << error.get()->what() << endl;
            }
		}
		if (!sourceUnit)
			return bytes();
	}
	catch(boost::exception const& _e)
	{
		auto msg = std::string("Parsing source code failed with: \n") + boost::diagnostic_information(_e);
		BOOST_FAIL(msg);
	}

	ErrorList errors;
	ErrorReporter errorReporter(errors);
	GlobalContext globalContext;
	Scoper::assignScopes(*sourceUnit);
	BOOST_REQUIRE(SolidityppSyntaxChecker(errorReporter, false).checkSyntax(*sourceUnit));
	NameAndTypeResolver resolver(globalContext, solidity::test::CommonOptions::get().evmVersion(), errorReporter);
	resolver.registerDeclarations(*sourceUnit);
	BOOST_REQUIRE_MESSAGE(resolver.resolveNamesAndTypes(*sourceUnit), "Resolving names failed");
	DeclarationTypeChecker declarationTypeChecker(errorReporter, solidity::test::CommonOptions::get().evmVersion());
	for (ASTPointer<ASTNode> const& node: sourceUnit->nodes())
		BOOST_REQUIRE(declarationTypeChecker.check(*node));
	TypeChecker typeChecker(solidity::test::CommonOptions::get().evmVersion(), errorReporter);
	BOOST_REQUIRE(typeChecker.checkTypeRequirements(*sourceUnit));
	for (ASTPointer<ASTNode> const& node: sourceUnit->nodes())
		if (ContractDefinition* contract = dynamic_cast<ContractDefinition*>(node.get()))
		{
			FirstExpressionExtractor extractor(*contract);
			BOOST_REQUIRE(extractor.expression() != nullptr);

			CompilerContext context(
				solidity::test::CommonOptions::get().evmVersion(),
				RevertStrings::Default
			);
			context.resetVisitedNodes(contract);
			context.setMostDerivedContract(*contract);
			context.setArithmetic(Arithmetic::Wrapping);
			size_t parametersSize = _localVariables.size(); // assume they are all one slot on the stack
			context.adjustStackOffset(static_cast<int>(parametersSize));
			for (vector<string> const& variable: _localVariables)
				context.addVariable(
					dynamic_cast<VariableDeclaration const&>(resolveDeclaration(*sourceUnit, variable, resolver)),
					static_cast<unsigned>(parametersSize--)
				);

			ExpressionCompiler(
				context,
				solidity::test::CommonOptions::get().optimize
			).compile(*extractor.expression());

			for (vector<string> const& function: _functions)
				context << context.functionEntryLabel(dynamic_cast<FunctionDefinition const&>(
					resolveDeclaration(*sourceUnit, function, resolver)
				));
			BOOST_REQUIRE(context.assemblyPtr());
			LinkerObject const& object = context.assemblyPtr()->assemble();
			BOOST_REQUIRE(object.immutableReferences.empty());
			bytes instructions = object.bytecode;
			// debug
			cout << evmasm::disassemble(instructions) << endl;
			return instructions;
		}
	BOOST_FAIL("No contract found in source.");
	return bytes();
}

} // end anonymous namespace


// Solidity++ related test
BOOST_AUTO_TEST_SUITE(SolidityppExpressionCompiler)

BOOST_AUTO_TEST_CASE(int_with_attov_vite_subdenomination)
{
	char const* sourceCode = R"(
		contract test {
			constructor() {
				 uint x = 1 attov;
			}
		}
	)";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({uint8_t(Instruction::PUSH1), 0x1});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(int_with_vite_vite_subdenomination)
{
	char const* sourceCode = R"(
		contract test {
			constructor() {
				 uint x = 1 vite;
			}
		}
	)";
	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({uint8_t(Instruction::PUSH8), 0xd, 0xe0, 0xb6, 0xb3, 0xa7, 0x64, 0x00, 0x00});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(height)
{
    char const* sourceCode = R"(
    contract test {
        constructor() {
                uint a = height();
            }
        }
    )";

    bytes code = compileFirstExpression(sourceCode);

    bytes expectation({uint8_t(Instruction::NUMBER)});
    BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(accountheight)
{
	char const* sourceCode = R"(
		contract test {
			constructor() {
				uint a = accountheight();
			}
		}
	)";

	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({uint8_t(Instruction::ACCOUNTHEIGHT)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(prevhash)
{
	char const* sourceCode = R"(
		contract test {
			constructor() {
				bytes32 a = prevhash();
			}
		}
	)";

	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({uint8_t(Instruction::PREVHASH)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(fromhash)
{
	char const* sourceCode = R"(
		contract test {
			constructor() {
				bytes32 a = fromhash();
			}
		}
	)";

	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({uint8_t(Instruction::FROMHASH)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(random64)
{
	char const* sourceCode = R"(
		contract test {
			constructor() {
				uint64 a = random64();
			}
		}
	)";

	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({uint8_t(Instruction::SEED)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(nextrandom)
{
	char const* sourceCode = R"(
		contract test {
			constructor() {
				uint64 a = nextrandom();
			}
		}
	)";

	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({uint8_t(Instruction::RANDOM)});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(blake2b_compute_at_compile_time)
{
	char const* sourceCode = R"(
		contract test {
			constructor() {
				bytes32 a = blake2b("abc");
			}
		}
	)";

	bytes code = compileFirstExpression(sourceCode);

	bytes expectation({
		uint8_t(Instruction::PUSH32), 0xbd, 0xdd, 0x81, 0x3c, 0x63, 0x42, 0x39, 0x72, 0x31, 0x71, 0xef, 0x3f, 0xee, 0x98, 0x57, 0x9b, 0x94, 0x96, 0x4e, 0x3b, 0xb1, 0xcb, 0x3e, 0x42, 0x72, 0x62, 0xc8, 0xc0, 0x68, 0xd5, 0x23, 0x19
		});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

BOOST_AUTO_TEST_CASE(blake2b)
{
	char const* sourceCode = R"(
		contract test {
			function f(bytes memory data) public {
				blake2b(data);
			}
        }
	)";

	bytes code = compileFirstExpression(sourceCode, {}, {{"test", "f", "data"}});

	bytes expectation({
		uint8_t(Instruction::DUP1),
        uint8_t(Instruction::DUP1),
        uint8_t(Instruction::MLOAD),
        uint8_t(Instruction::SWAP1),
        uint8_t(Instruction::PUSH1), 0x20,
        uint8_t(Instruction::ADD),
        uint8_t(Instruction::BLAKE2B)
		});
	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

 BOOST_AUTO_TEST_CASE(msg_tokenid)
 {
 	char const* sourceCode = R"(
 		contract test {
 			function f() external {
 	            vitetoken t = msg.token;
 			}
 		}
 	)";

 	bytes code = compileFirstExpression(sourceCode, {}, {});

 	bytes expectation({uint8_t(Instruction::TOKENID)});
 	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
 }

BOOST_AUTO_TEST_CASE(msg_amount)
{
    char const* sourceCode = R"(
        contract test {
            function f() external {
                uint a = msg.value;
            }
        }
    )";

    bytes code = compileFirstExpression(sourceCode, {}, {});

    bytes expectation({uint8_t(Instruction::CALLVALUE)});
    BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
}

 BOOST_AUTO_TEST_CASE(balance)
 {
 	char const* sourceCode = R"(
 		contract test {
 			function f(vitetoken token) public {
 				balance(token);
 			}
 		}
 	)";

 	bytes code = compileFirstExpression(sourceCode, {}, {{"test", "f", "token"}});

 	bytes expectation({
 	    uint8_t(Instruction::DUP1),
 	    uint8_t(Instruction::BALANCE)
 	});
 	BOOST_CHECK_EQUAL_COLLECTIONS(code.begin(), code.end(), expectation.begin(), expectation.end());
 }
BOOST_AUTO_TEST_SUITE_END()

} // end namespaces
