#include <test/libsolidity/AnalysisFramework.h>

#include <test/Common.h>

#include <libsolidity/ast/AST.h>

#include <libsolutil/Keccak256.h>

#include <boost/test/unit_test.hpp>

#include <string>

using namespace std;
using namespace solidity::langutil;

namespace solidity::frontend::test
{

BOOST_FIXTURE_TEST_SUITE(SolidityppNameAndTypeResolution, AnalysisFramework)

BOOST_AUTO_TEST_CASE(onmessage_canonical_signature_type_aliases)
{
	SourceUnit const* sourceUnit = nullptr;
	char const* text = R"(
		contract Test {
			onMessage boo(uint, bytes32, address) {
			}
		}
	)";
	sourceUnit = parseAndAnalyse(text);
	for (ASTPointer<ASTNode> const& node: sourceUnit->nodes())
		if (ContractDefinition* contract = dynamic_cast<ContractDefinition*>(node.get()))
		{
			auto functions = contract->definedFunctions();
			if (functions.empty())
				continue;
			BOOST_CHECK_EQUAL("boo(uint256,bytes32,address)", functions[0]->externalSignature());
		}
}

BOOST_AUTO_TEST_SUITE_END()

} // end namespaces
