// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Full-stack compiler that converts a source code string to bytecode.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#pragma once

#include <libsolidity/interface/ReadFile.h>
#include <libsolidity/interface/OptimiserSettings.h>
#include <libsolidity/interface/Version.h>
#include <libsolidity/interface/DebugSettings.h>

#include <libsolidity/formal/ModelCheckerSettings.h>

#include <libsmtutil/SolverInterface.h>

#include <liblangutil/ErrorReporter.h>
#include <liblangutil/EVMVersion.h>
#include <liblangutil/SourceLocation.h>

#include <libevmasm/LinkerObject.h>

#include <libsolutil/Common.h>
#include <libsolutil/FixedHash.h>
#include <libsolutil/LazyInit.h>

#include <boost/noncopyable.hpp>
#include <json/json.h>

#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace solidity::langutil
{
class Scanner;
}


namespace solidity::evmasm
{
class Assembly;
class AssemblyItem;
using AssemblyItems = std::vector<AssemblyItem>;
}

namespace solidity::frontend
{

// forward declarations
class ASTNode;
class ContractDefinition;
class FunctionDefinition;
class SourceUnit;
class Compiler;
class GlobalContext;
class Natspec;
class DeclarationContainer;

/**
 * Easy to use and self-contained Solidity compiler with as few header dependencies as possible.
 * It holds state and can be used to either step through the compilation stages (and abort e.g.
 * before compilation to bytecode) or run the whole compilation in one call.
 * If error recovery is active, it is possible to progress through the stages even when
 * there are errors. In any case, producing code is only possible without errors.
 */
class CompilerStack: boost::noncopyable
{
public:
	enum State {
		Empty,
		SourcesSet,
		Parsed,
		ParsedAndImported,
		AnalysisPerformed,
		CompilationSuccessful
	};

	enum class MetadataHash {
		IPFS,
		Bzzr1,
		None
	};

	struct Remapping
	{
		std::string context;
		std::string prefix;
		std::string target;
	};

	/// Creates a new compiler stack.
	/// @param _readFile callback used to read files for import statements. Must return
	/// and must not emit exceptions.
	explicit CompilerStack(ReadCallback::Callback _readFile = ReadCallback::Callback(), bool _verbose = false);

	~CompilerStack();

	/// @returns the list of errors that occurred during parsing and type checking.
	langutil::ErrorList const& errors() const { return m_errorReporter.errors(); }

	/// @returns the current state.
	State state() const { return m_stackState; }

	bool hasError() const { return m_hasError; }

	bool compilationSuccessful() const { return m_stackState >= CompilationSuccessful; }

	/// Resets the compiler to an empty state. Unless @a _keepSettings is set to true,
	/// all settings are reset as well.
	void reset(bool _keepSettings = false);

	// Parses a remapping of the format "context:prefix=target".
	static std::optional<Remapping> parseRemapping(std::string const& _remapping);

	/// Sets path remappings.
	/// Must be set before parsing.
	void setRemappings(std::vector<Remapping> const& _remappings);

	/// Sets library addresses. Addresses are cleared iff @a _libraries is missing.
	/// Must be set before parsing.
	void setLibraries(std::map<std::string, util::h160> const& _libraries = {});

	/// Changes the optimiser settings.
	/// Must be set before parsing.
	void setOptimiserSettings(bool _optimize, unsigned _runs = 200);

	/// Changes the optimiser settings.
	/// Must be set before parsing.
	void setOptimiserSettings(OptimiserSettings _settings);

	/// Sets whether to strip revert strings, add additional strings or do nothing at all.
	void setRevertStringBehaviour(RevertStrings _revertStrings);

	/// Set whether or not parser error is desired.
	/// When called without an argument it will revert to the default.
	/// Must be set before parsing.
	void setParserErrorRecovery(bool _wantErrorRecovery = false)
	{
		m_parserErrorRecovery = _wantErrorRecovery;
	}

	/// Sets the pipeline to go through the Yul IR or not.
	/// Must be set before parsing.
	void setViaIR(bool _viaIR);

	/// Set the EVM version used before running compile.
	/// When called without an argument it will revert to the default version.
	/// Must be set before parsing.
	void setEVMVersion(langutil::EVMVersion _version = langutil::EVMVersion{});

	/// Set model checker settings.
	void setModelCheckerSettings(ModelCheckerSettings _settings);
	/// Set which SMT solvers should be enabled.
	void setSMTSolverChoice(smtutil::SMTSolverChoice _enabledSolvers);

	/// Sets the requested contract names by source.
	/// If empty, no filtering is performed and every contract
	/// found in the supplied sources is compiled.
	/// Names are cleared iff @a _contractNames is missing.
	void setRequestedContractNames(std::map<std::string, std::set<std::string>> const& _contractNames = std::map<std::string, std::set<std::string>>{})
	{
		m_requestedContractNames = _contractNames;
	}

	/// Enable EVM Bytecode generation. This is enabled by default.
	void enableEvmBytecodeGeneration(bool _enable = true) { m_generateEvmBytecode = _enable; }

	/// Enable experimental generation of Yul IR code.
	void enableIRGeneration(bool _enable = true) { m_generateIR = _enable; }

	/// Enable experimental generation of Ewasm code. If enabled, IR is also generated.
	void enableEwasmGeneration(bool _enable = true) { m_generateEwasm = _enable; }

	/// @arg _metadataLiteralSources When true, store sources as literals in the contract metadata.
	/// Must be set before parsing.
	void useMetadataLiteralSources(bool _metadataLiteralSources);

	/// Sets whether and which hash should be used
	/// to store the metadata in the bytecode.
	/// @param _metadataHash can be IPFS, Bzzr1, None
	void setMetadataHash(MetadataHash _metadataHash);

	/// Sets the sources. Must be set before parsing.
	void setSources(StringMap _sources);

	/// Adds a response to an SMTLib2 query (identified by the hash of the query input).
	/// Must be set before parsing.
	void addSMTLib2Response(util::h256 const& _hash, std::string const& _response);

	/// Parses all source units that were added
	/// @returns false on error.
	bool parse();

	/// Imports given SourceUnits so they can be analyzed. Leads to the same internal state as parse().
	/// Will throw errors if the import fails
	void importASTs(std::map<std::string, Json::Value> const& _sources);

	/// Performs the analysis steps (imports, scopesetting, syntaxCheck, referenceResolving,
	///  typechecking, staticAnalysis) on previously parsed sources.
	/// @returns false on error.
	bool analyze();

	/// Parses and analyzes all source units that were added
	/// @returns false on error.
	bool parseAndAnalyze(State _stopAfter = State::CompilationSuccessful);

	/// Compiles the source units that were previously added and parsed.
	/// @returns false on error.
	bool compile(State _stopAfter = State::CompilationSuccessful);

	/// @returns the list of sources (paths) used
	std::vector<std::string> sourceNames() const;

	/// @returns a mapping assigning each source name its index inside the vector returned
	/// by sourceNames().
	std::map<std::string, unsigned> sourceIndices() const;

	/// @returns the previously used scanner, useful for counting lines during error reporting.
	langutil::Scanner const& scanner(std::string const& _sourceName) const;

	/// @returns the parsed source unit with the supplied name.
	SourceUnit const& ast(std::string const& _sourceName) const;

	/// Helper function for logs printing. Do only use in error cases, it's quite expensive.
	/// line and columns are numbered starting from 1 with following order:
	/// start line, start column, end line, end column
	std::tuple<int, int, int, int> positionFromSourceLocation(langutil::SourceLocation const& _sourceLocation) const;

	/// @returns a list of unhandled queries to the SMT solver (has to be supplied in a second run
	/// by calling @a addSMTLib2Response).
	std::vector<std::string> const& unhandledSMTLib2Queries() const { return m_unhandledSMTLib2Queries; }

	/// @returns a list of the contract names in the sources.
	std::vector<std::string> contractNames() const;

	/// @returns the name of the last contract.
	std::string const lastContractName() const;

	/// @returns either the contract's name or a mixture of its name and source file, sanitized for filesystem use
	std::string const filesystemFriendlyName(std::string const& _contractName) const;

	/// @returns the IR representation of a contract.
	std::string const& yulIR(std::string const& _contractName) const;

	/// @returns the optimized IR representation of a contract.
	std::string const& yulIROptimized(std::string const& _contractName) const;

	/// @returns the Ewasm text representation of a contract.
	std::string const& ewasm(std::string const& _contractName) const;

	/// @returns the Ewasm representation of a contract.
	evmasm::LinkerObject const& ewasmObject(std::string const& _contractName) const;

	/// @returns the assembled object for a contract.
	evmasm::LinkerObject const& object(std::string const& _contractName) const;

	/// @returns the runtime object for the contract.
	evmasm::LinkerObject const& runtimeObject(std::string const& _contractName) const;

	/// @returns normal contract assembly items
	evmasm::AssemblyItems const* assemblyItems(std::string const& _contractName) const;

	/// @returns runtime contract assembly items
	evmasm::AssemblyItems const* runtimeAssemblyItems(std::string const& _contractName) const;

	/// @returns an array containing all utility sources generated during compilation.
	/// Format: [ { name: string, id: number, language: "Yul", contents: string }, ... ]
	Json::Value generatedSources(std::string const& _contractName, bool _runtime = false) const;

	/// @returns the string that provides a mapping between bytecode and sourcecode or a nullptr
	/// if the contract does not (yet) have bytecode.
	std::string const* sourceMapping(std::string const& _contractName) const;

	/// @returns the string that provides a mapping between runtime bytecode and sourcecode or a nullptr
	/// if the contract does not (yet) have bytecode.
	std::string const* runtimeSourceMapping(std::string const& _contractName) const;

	/// @return a verbose text representation of the assembly.
	/// @arg _sourceCodes is the map of input files to source code strings
	/// Prerequisite: Successful compilation.
	std::string assemblyString(std::string const& _contractName, StringMap _sourceCodes = StringMap()) const;

	/// @returns a JSON representation of the assembly.
	/// @arg _sourceCodes is the map of input files to source code strings
	/// Prerequisite: Successful compilation.
	Json::Value assemblyJSON(std::string const& _contractName) const;

	/// @returns a JSON representing the contract ABI.
	/// Prerequisite: Successful call to parse or compile.
	Json::Value const& contractABI(std::string const& _contractName) const;

	/// @returns a JSON representing the storage layout of the contract.
	/// Prerequisite: Successful call to parse or compile.
	Json::Value const& storageLayout(std::string const& _contractName) const;

	/// @returns a JSON representing the contract's user documentation.
	/// Prerequisite: Successful call to parse or compile.
	Json::Value const& natspecUser(std::string const& _contractName) const;

	/// @returns a JSON representing the contract's developer documentation.
	/// Prerequisite: Successful call to parse or compile.
	Json::Value const& natspecDev(std::string const& _contractName) const;

	/// @returns a JSON representing a map of method identifiers (hashes) to function names.
	Json::Value methodIdentifiers(std::string const& _contractName) const;

	/// @returns the Contract Metadata
	std::string const& metadata(std::string const& _contractName) const;

	/// @returns the cbor-encoded metadata.
	bytes cborMetadata(std::string const& _contractName) const;

	/// @returns a JSON representing the estimated gas usage for contract creation, internal and external functions
	Json::Value gasEstimates(std::string const& _contractName) const;

	/// Overwrites the release/prerelease flag. Should only be used for testing.
	void overwriteReleaseFlag(bool release) { m_release = release; }
private:
	/// The state per source unit. Filled gradually during parsing.
	struct Source
	{
		std::shared_ptr<langutil::Scanner> scanner;
		std::shared_ptr<SourceUnit> ast;
		util::h256 mutable keccak256HashCached;
		util::h256 mutable swarmHashCached;
		std::string mutable ipfsUrlCached;
		void reset() { *this = Source(); }
		util::h256 const& keccak256() const;
		util::h256 const& swarmHash() const;
		std::string const& ipfsUrl() const;
	};

	/// The state per contract. Filled gradually during compilation.
	struct Contract
	{
		ContractDefinition const* contract = nullptr;
		std::shared_ptr<Compiler> compiler;
		std::shared_ptr<evmasm::Assembly> evmAssembly;
		std::shared_ptr<evmasm::Assembly> evmRuntimeAssembly;
		evmasm::LinkerObject object; ///< Deployment object (includes the runtime sub-object).
		evmasm::LinkerObject runtimeObject; ///< Runtime object.

		std::string yulIR; ///< Experimental Yul IR code.
		std::string yulIROptimized; ///< Optimized experimental Yul IR code.
		std::string ewasm; ///< Experimental Ewasm text representation
		evmasm::LinkerObject ewasmObject; ///< Experimental Ewasm code
		util::LazyInit<std::string const> metadata; ///< The metadata json that will be hashed into the chain.
		util::LazyInit<Json::Value const> abi;
		util::LazyInit<Json::Value const> storageLayout;
		util::LazyInit<Json::Value const> userDocumentation;
		util::LazyInit<Json::Value const> devDocumentation;
		util::LazyInit<Json::Value const> generatedSources;
		util::LazyInit<Json::Value const> runtimeGeneratedSources;
		mutable std::optional<std::string const> sourceMapping;
		mutable std::optional<std::string const> runtimeSourceMapping;
	};

	/// Loads the missing sources from @a _ast (named @a _path) using the callback
	/// @a m_readFile and stores the absolute paths of all imports in the AST annotations.
	/// @returns the newly loaded sources.
	StringMap loadMissingSources(SourceUnit const& _ast, std::string const& _path);
	std::string applyRemapping(std::string const& _path, std::string const& _context);
	void resolveImports();

	/// Store the contract definitions in m_contracts.
	void storeContractDefinitions();

	/// @returns true if the source is requested to be compiled.
	bool isRequestedSource(std::string const& _sourceName) const;

	/// @returns true if the contract is requested to be compiled.
	bool isRequestedContract(ContractDefinition const& _contract) const;

	/// Compile a single contract.
	/// @param _otherCompilers provides access to compilers of other contracts, to get
	///                        their bytecode if needed. Only filled after they have been compiled.
	void compileContract(
		ContractDefinition const& _contract,
		std::map<ContractDefinition const*, std::shared_ptr<Compiler const>>& _otherCompilers
	);

	/// Generate Yul IR for a single contract.
	/// The IR is stored but otherwise unused.
	void generateIR(ContractDefinition const& _contract);

	/// Generate EVM representation for a single contract.
	/// Depends on output generated by generateIR.
	void generateEVMFromIR(ContractDefinition const& _contract);

	/// Generate Ewasm representation for a single contract.
	/// Depends on output generated by generateIR.
	void generateEwasm(ContractDefinition const& _contract);

	/// Links all the known library addresses in the available objects. Any unknown
	/// library will still be kept as an unlinked placeholder in the objects.
	void link();

	/// @returns the contract object for the given @a _contractName.
	/// Can only be called after state is CompilationSuccessful.
	Contract const& contract(std::string const& _contractName) const;

	/// @returns the source object for the given @a _sourceName.
	/// Can only be called after state is SourcesSet.
	Source const& source(std::string const& _sourceName) const;

	/// @returns the parsed contract with the supplied name. Throws an exception if the contract
	/// does not exist.
	ContractDefinition const& contractDefinition(std::string const& _contractName) const;

	/// @returns the metadata JSON as a compact string for the given contract.
	std::string createMetadata(Contract const& _contract) const;

	/// @returns the metadata CBOR for the given serialised metadata JSON.
	bytes createCBORMetadata(Contract const& _contract) const;

	/// @returns the contract ABI as a JSON object.
	/// This will generate the JSON object and store it in the Contract object if it is not present yet.
	Json::Value const& contractABI(Contract const&) const;

	/// @returns the storage layout of the contract as a JSON object.
	/// This will generate the JSON object and store it in the Contract object if it is not present yet.
	Json::Value const& storageLayout(Contract const&) const;

	/// @returns the Natspec User documentation as a JSON object.
	/// This will generate the JSON object and store it in the Contract object if it is not present yet.
	Json::Value const& natspecUser(Contract const&) const;

	/// @returns the Natspec Developer documentation as a JSON object.
	/// This will generate the JSON object and store it in the Contract object if it is not present yet.
	Json::Value const& natspecDev(Contract const&) const;

	/// @returns the Contract Metadata
	/// This will generate the metadata and store it in the Contract object if it is not present yet.
	std::string const& metadata(Contract const&) const;

	/// @returns the offset of the entry point of the given function into the list of assembly items
	/// or zero if it is not found or does not exist.
	size_t functionEntryPoint(
		std::string const& _contractName,
		FunctionDefinition const& _function
	) const;

	/// Solidity++: output debug info in verbose mode
	void debug(std::string info) const { if (m_verbose) std::clog << "[CompilerStack] " << info << std::endl; }

	ReadCallback::Callback m_readFile;
	OptimiserSettings m_optimiserSettings;
	RevertStrings m_revertStrings = RevertStrings::Default;
	State m_stopAfter = State::CompilationSuccessful;
	bool m_viaIR = false;
	langutil::EVMVersion m_evmVersion;
	ModelCheckerSettings m_modelCheckerSettings;
	smtutil::SMTSolverChoice m_enabledSMTSolvers;
	std::map<std::string, std::set<std::string>> m_requestedContractNames;
	bool m_generateEvmBytecode = true;
	bool m_generateIR = false;
	bool m_generateEwasm = false;
	std::map<std::string, util::h160> m_libraries;
	/// list of path prefix remappings, e.g. mylibrary: github.com/ethereum = /usr/local/ethereum
	/// "context:prefix=target"
	std::vector<Remapping> m_remappings;
	std::map<std::string const, Source> m_sources;
	// if imported, store AST-JSONS for each filename
	std::map<std::string, Json::Value> m_sourceJsons;
	std::vector<std::string> m_unhandledSMTLib2Queries;
	std::map<util::h256, std::string> m_smtlib2Responses;
	std::shared_ptr<GlobalContext> m_globalContext;
	std::vector<Source const*> m_sourceOrder;
	std::map<std::string const, Contract> m_contracts;
	langutil::ErrorList m_errorList;
	langutil::ErrorReporter m_errorReporter;
	bool m_metadataLiteralSources = false;
	MetadataHash m_metadataHash = MetadataHash::IPFS;
	bool m_parserErrorRecovery = false;
	State m_stackState = Empty;
	bool m_importedSources = false;
	/// Whether or not there has been an error during processing.
	/// If this is true, the stack will refuse to generate code.
	bool m_hasError = false;
	bool m_release = VersionIsRelease;
	bool m_verbose = false;  // Solidity++
};

}
