#include <test/Common.h>
#include <test/TestCase.h>

#include <libsolutil/AnsiColorized.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <iostream>
#include <stdexcept>

using namespace std;
using namespace solidity;
using namespace solidity::frontend;
using namespace solidity::frontend::test;

void TestCase::printSettings(ostream& _stream, const string& _linePrefix, const bool)
{
	auto& settings = m_reader.settings();
	if (settings.empty())
		return;

	_stream << _linePrefix << "// ====" << endl;
	for (auto const& setting: settings)
		_stream << _linePrefix << "// " << setting.first << ": " << setting.second << endl;
}

void TestCase::printUpdatedSettings(std::ostream& _stream, std::string const& _linePrefix)
{
	printSettings(_stream, _linePrefix);
}

bool TestCase::isTestFilename(boost::filesystem::path const& _filename)
{
	string extension = _filename.extension().string();
	return (extension == ".sol" || extension == ".yul" || extension == ".solpp") &&
		   !boost::starts_with(_filename.string(), "~") &&
			!boost::starts_with(_filename.string(), ".");
}

bool TestCase::shouldRun()
{
	m_reader.ensureAllSettingsRead();
	return m_shouldRun;
}

void TestCase::expect(string::iterator& _it, string::iterator _end, string::value_type _c)
{
	if (_it == _end || *_it != _c)
		throw runtime_error(string("Invalid test expectation. Expected: \"") + _c + "\".");
	++_it;
}

void TestCase::printIndented(ostream& _stream, string const& _output, string const& _linePrefix) const
{
	stringstream output(_output);
	string line;
	while (getline(output, line))
		if (line.empty())
			// Avoid trailing spaces.
			_stream << boost::trim_right_copy(_linePrefix) << endl;
		else
			_stream << _linePrefix << line << endl;
}

void TestCase::printSource(ostream& _stream, string const& _linePrefix, bool const) const
{
	printIndented(_stream, m_source, _linePrefix);
}

void TestCase::printUpdatedExpectations(ostream& _stream, string const& _linePrefix) const
{
	printIndented(_stream, m_obtainedResult, _linePrefix);
}

TestCase::TestResult TestCase::checkResult(std::ostream& _stream, const std::string& _linePrefix, bool const _formatted)
{
	if (m_expectation != m_obtainedResult)
	{
		string nextIndentLevel = _linePrefix + "  ";
		util::AnsiColorized(_stream, _formatted, {util::formatting::BOLD, util::formatting::CYAN})
			<< _linePrefix << "Expected result:" << endl;
		// TODO could compute a simple diff with highlighted lines
		printIndented(_stream, m_expectation, nextIndentLevel);
		util::AnsiColorized(_stream, _formatted, {util::formatting::BOLD, util::formatting::CYAN})
			<< _linePrefix << "Obtained result:" << endl;
		printIndented(_stream, m_obtainedResult, nextIndentLevel);
		return TestResult::Failure;
	}
	return TestResult::Success;
}

EVMVersionRestrictedTestCase::EVMVersionRestrictedTestCase(string const& _filename):
	TestCase(_filename)
{
	string versionString = m_reader.stringSetting("EVMVersion", "any");
	if (versionString == "any")
		return;

	string comparator;
	size_t versionBegin = 0;
	for (auto character: versionString)
		if (!isalpha(character))
		{
			comparator += character;
			versionBegin++;
		}
		else
			break;

	versionString = versionString.substr(versionBegin);
	std::optional<langutil::EVMVersion> version = langutil::EVMVersion::fromString(versionString);
	if (!version)
		BOOST_THROW_EXCEPTION(runtime_error{"Invalid EVM version: \"" + versionString + "\""});

	langutil::EVMVersion evmVersion = solidity::test::CommonOptions::get().evmVersion();
	bool comparisonResult;
	if (comparator == ">")
		comparisonResult = evmVersion > version;
	else if (comparator == ">=")
		comparisonResult = evmVersion >= version;
	else if (comparator == "<")
		comparisonResult = evmVersion < version;
	else if (comparator == "<=")
		comparisonResult = evmVersion <= version;
	else if (comparator == "=")
		comparisonResult = evmVersion == version;
	else if (comparator == "!")
		comparisonResult = !(evmVersion == version);
	else
		BOOST_THROW_EXCEPTION(runtime_error{"Invalid EVM comparator: \"" + comparator + "\""});

	if (!comparisonResult)
		m_shouldRun = false;
}
