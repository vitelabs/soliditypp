// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ command line interface.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */
#pragma once

#include <libsolidity/interface/CompilerStack.h>
#include <libsolidity/interface/DebugSettings.h>
#include <libyul/AssemblyStack.h>
#include <liblangutil/EVMVersion.h>

#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>

#include <memory>

namespace solidity::frontend
{

//forward declaration
enum class DocumentationType: uint8_t;

class CommandLineInterface
{
public:
	/// Parse command line arguments and return false if we should not continue
	bool parseArguments(int _argc, char** _argv);
	/// Parse the files and create source code objects
	bool processInput();
	/// Perform actions on the input depending on provided compiler arguments
	/// @returns true on success.
	bool actOnInput();

private:
	bool link();
	void writeLinkedFiles();
	/// @returns the ``// <identifier> -> name`` hint for library placeholders.
	static std::string libraryPlaceholderHint(std::string const& _libraryName);
	/// @returns the full object with library placeholder hints in hex.
	static std::string objectWithLinkRefsHex(evmasm::LinkerObject const& _obj);

	bool assemble(
		yul::AssemblyStack::Language _language,
		yul::AssemblyStack::Machine _targetMachine,
		bool _optimize,
		std::optional<std::string> _yulOptimiserSteps = std::nullopt
	);

	void outputCompilationResults();

	void handleCombinedJSON();
	void handleAst();
	void handleBinary(std::string const& _contract);
	void handleOpcode(std::string const& _contract);
	void handleIR(std::string const& _contract);
	void handleIROptimized(std::string const& _contract);
	void handleEwasm(std::string const& _contract);
	void handleBytecode(std::string const& _contract);
	void handleSignatureHashes(std::string const& _contract);
	void handleMetadata(std::string const& _contract);
	void handleABI(std::string const& _contract);
	void handleNatspec(bool _natspecDev, std::string const& _contract);
	void handleGasEstimation(std::string const& _contract);
	void handleStorageLayout(std::string const& _contract);

	/// Fills @a m_sourceCodes initially and @a m_redirects.
	bool readInputFilesAndConfigureRemappings();
	/// Tries to read from the file @a _input or interprets _input literally if that fails.
	/// It then tries to parse the contents and appends to m_libraries.
	bool parseLibraryOption(std::string const& _input);

	/// Tries to read @ m_sourceCodes as a JSONs holding ASTs
	/// such that they can be imported into the compiler  (importASTs())
	/// (produced by --combined-json ast,compact-format <file.sol>
	/// or standard-json output
	std::map<std::string, Json::Value> parseAstFromInput();

	/// Create a file in the given directory
	/// @arg _fileName the name of the file
	/// @arg _data to be written
	void createFile(std::string const& _fileName, std::string const& _data);

	/// Create a json file in the given directory
	/// @arg _fileName the name of the file (the extension will be replaced with .json)
	/// @arg _json json string to be written
	void createJson(std::string const& _fileName, std::string const& _json);

	size_t countEnabledOptions(std::vector<std::string> const& _optionNames) const;
	static std::string joinOptionNames(std::vector<std::string> const& _optionNames, std::string _separator = ", ");

	bool m_error = false; ///< If true, some error occurred.

	bool m_onlyAssemble = false;

	bool m_onlyLink = false;

	/// Compiler arguments variable map
	boost::program_options::variables_map m_args;
	/// map of input files to source code strings
	std::map<std::string, std::string> m_sourceCodes;
	/// list of remappings
	std::vector<frontend::CompilerStack::Remapping> m_remappings;
	/// list of allowed directories to read files from
	std::vector<boost::filesystem::path> m_allowedDirectories;
	/// Base path, used for resolving relative paths in imports.
	boost::filesystem::path m_basePath;
	/// map of library names to addresses
	std::map<std::string, util::h168> m_libraries;  // Solidity++: 168-bit address
	/// Solidity compiler stack
	std::unique_ptr<frontend::CompilerStack> m_compiler;
	CompilerStack::State m_stopAfter = CompilerStack::State::CompilationSuccessful;
	/// EVM version to use
	langutil::EVMVersion m_evmVersion;
	/// How to handle revert strings
	RevertStrings m_revertStrings = RevertStrings::Default;
	/// Chosen hash method for the bytecode metadata.
	CompilerStack::MetadataHash m_metadataHash = CompilerStack::MetadataHash::IPFS;
	/// Model checker settings.
	ModelCheckerSettings m_modelCheckerSettings;
	/// Whether or not to colorize diagnostics output.
	bool m_coloredOutput = true;
	/// Whether or not to output error IDs.
	bool m_withErrorIds = false;
};

}
