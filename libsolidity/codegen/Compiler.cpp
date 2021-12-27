// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ compiler.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#include <libsolidity/codegen/Compiler.h>

#include <libsolidity/codegen/ContractCompiler.h>
#include <libevmasm/Assembly.h>

using namespace std;
using namespace solidity;
using namespace solidity::frontend;

void Compiler::compileContract(
	ContractDefinition const& _contract,
	std::map<ContractDefinition const*, shared_ptr<Compiler const>> const& _otherCompilers,
	bytes const& _metadata
)
{
	ContractCompiler runtimeCompiler(nullptr, m_runtimeContext, m_optimiserSettings, m_verbose);
	runtimeCompiler.compileContract(_contract, _otherCompilers);

	m_runtimeContext.appendAuxiliaryData(_metadata);

	// This might modify m_runtimeContext because it can access runtime functions at
	// creation time.
	OptimiserSettings creationSettings{m_optimiserSettings};
	// The creation code will be executed at most once, so we modify the optimizer
	// settings accordingly.
	creationSettings.expectedExecutionsPerDeployment = 1;
	ContractCompiler creationCompiler(&runtimeCompiler, m_context, creationSettings);
	m_runtimeSub = creationCompiler.compileConstructor(_contract, _otherCompilers);

	m_context.optimise(m_optimiserSettings);

	solAssert(m_context.appendYulUtilityFunctionsRan(), "appendYulUtilityFunctions() was not called in compiler context.");
	solAssert(m_runtimeContext.appendYulUtilityFunctionsRan(), "appendYulUtilityFunctions() was not called in runtime compiler context.");
}

// Solidity++: compile Vite contract (without metadata)
void Compiler::compileViteContract(
	ContractDefinition const& _contract,
	std::map<ContractDefinition const*, shared_ptr<Compiler const>> const& _otherCompilers
)
{
    debug("Compiling runtime");
	ContractCompiler runtimeCompiler(nullptr, m_runtimeContext, m_optimiserSettings, m_verbose);
	runtimeCompiler.compileContract(_contract, _otherCompilers);

	// @todo: Append real metadata
    // Solidity++: Append placeholder only for compatibility with the old version of Vite VM
    bytes _metadata = {0xa1, 0x65, 'b', 'z', 'z', 'r', '0', 0x58, 0x20,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x29};
    m_runtimeContext.appendAuxiliaryData(_metadata);

	// This might modify m_runtimeContext because it can access runtime functions at
	// creation time.
	OptimiserSettings creationSettings{m_optimiserSettings};
	// The creation code will be executed at most once, so we modify the optimizer
	// settings accordingly.
	creationSettings.expectedExecutionsPerDeployment = 1;
	ContractCompiler creationCompiler(&runtimeCompiler, m_context, creationSettings, m_verbose);
	debug("Compiling constructor");
	m_runtimeSub = creationCompiler.compileConstructor(_contract, _otherCompilers);

	m_context.optimise(m_optimiserSettings);

	solAssert(m_context.appendYulUtilityFunctionsRan(), "appendYulUtilityFunctions() was not called in compiler context.");
	solAssert(m_runtimeContext.appendYulUtilityFunctionsRan(), "appendYulUtilityFunctions() was not called in runtime compiler context.");
	debug("Compiled.");
}

std::shared_ptr<evmasm::Assembly> Compiler::runtimeAssemblyPtr() const
{
	solAssert(m_context.runtimeContext(), "");
	return m_context.runtimeContext()->assemblyPtr();
}

evmasm::AssemblyItem Compiler::functionEntryLabel(FunctionDefinition const& _function) const
{
	return m_runtimeContext.functionEntryLabelIfExists(_function);
}
