// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ compiler.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#pragma once

#include <libsolidity/codegen/CompilerContext.h>
#include <libsolidity/interface/OptimiserSettings.h>
#include <libsolidity/interface/DebugSettings.h>
#include <liblangutil/EVMVersion.h>
#include <libevmasm/Assembly.h>
#include <functional>
#include <ostream>

namespace solidity::frontend {

class Compiler
{
public:
	Compiler(langutil::EVMVersion _evmVersion, RevertStrings _revertStrings, OptimiserSettings _optimiserSettings):
		m_optimiserSettings(std::move(_optimiserSettings)),
		m_runtimeContext(_evmVersion, _revertStrings),
		m_context(_evmVersion, _revertStrings, &m_runtimeContext),
		m_offchainContext(_evmVersion, _revertStrings)
	{ }

	/// Compiles a contract.
	/// @arg _metadata contains the to be injected metadata CBOR
	void compileContract(
		ContractDefinition const& _contract,
		std::map<ContractDefinition const*, std::shared_ptr<Compiler const>> const& _otherCompilers,
		bytes const& _metadata
	);
	/// @returns Entire assembly.
	evmasm::Assembly const& assembly() const { return m_context.assembly(); }
	/// @returns Runtime assembly.
	evmasm::Assembly const& runtimeAssembly() const { return m_context.assembly().sub(m_runtimeSub); }

	/// @returns Offchain assembly.
	/// Solidity++: get offchain assembly
	evmasm::Assembly const& offchainAssembly() const { return m_offchainContext.assembly(); }

	/// @returns Entire assembly as a shared pointer to non-const.
	std::shared_ptr<evmasm::Assembly> assemblyPtr() const { return m_context.assemblyPtr(); }
	/// @returns Runtime assembly as a shared pointer.
	std::shared_ptr<evmasm::Assembly> runtimeAssemblyPtr() const;

	/// @returns Offchain assembly as a shared pointer to non-const.
	/// Solidity++: get offchain assembly pointer
	std::shared_ptr<evmasm::Assembly> offchainAssemblyPtr() const { return m_offchainContext.assemblyPtr(); }

	std::string generatedYulUtilityCode() const { return m_context.generatedYulUtilityCode(); }
	std::string runtimeGeneratedYulUtilityCode() const { return m_runtimeContext.generatedYulUtilityCode(); }

	/// @returns the entry label of the given function. Might return an AssemblyItem of type
	/// UndefinedItem if it does not exist yet.
	evmasm::AssemblyItem functionEntryLabel(FunctionDefinition const& _function) const;

private:
	OptimiserSettings const m_optimiserSettings;
	CompilerContext m_runtimeContext;
	size_t m_runtimeSub = size_t(-1); ///< Identifier of the runtime sub-assembly, if present.
	CompilerContext m_context;
	// Solidity++: offchain compiler context
	CompilerContext m_offchainContext;
};

}
