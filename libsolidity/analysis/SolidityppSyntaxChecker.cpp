// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ syntax checker
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#include <libsolidity/analysis/SolidityppSyntaxChecker.h>

#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/ExperimentalFeatures.h>
#include <libsolidity/interface/Version.h>

#include <libyul/AST.h>

#include <liblangutil/ErrorReporter.h>
#include <liblangutil/SemVerHandler.h>

#include <memory>
#include <string>

using namespace std;
using namespace solidity;
using namespace solidity::langutil;
using namespace solidity::frontend;
using namespace solidity::util;

void SolidityppSyntaxChecker::endVisit(SourceUnit const& _sourceUnit)
{
	if (!m_versionPragmaFound)
	{
		string errorString("Source file does not specify required compiler version!");
		SemVerVersion recommendedVersion{string(VersionString)};
		if (!recommendedVersion.isPrerelease())
			errorString +=
				" Consider adding \"pragma soliditypp ^" +
				to_string(recommendedVersion.major()) +
				string(".") +
				to_string(recommendedVersion.minor()) +
				string(".") +
				to_string(recommendedVersion.patch()) +
				string(";\"");

		// when reporting the warning, print the source name only
		m_errorReporter.warning(3420_error, {-1, -1, _sourceUnit.location().source}, errorString);
	}
	if (!m_sourceUnit->annotation().useABICoderV2.set())
		m_sourceUnit->annotation().useABICoderV2 = true;

	m_sourceUnit = nullptr;
}

bool SolidityppSyntaxChecker::visit(PragmaDirective const& _pragma)
{
	solAssert(!_pragma.tokens().empty(), "");
	solAssert(_pragma.tokens().size() == _pragma.literals().size(), "");
	if (_pragma.tokens()[0] != Token::Identifier)
		m_errorReporter.syntaxError(5226_error, _pragma.location(), "Invalid pragma \"" + _pragma.literals()[0] + "\"");
	else if (_pragma.literals()[0] == "experimental")
	{
		solAssert(m_sourceUnit, "");
		vector<string> literals(_pragma.literals().begin() + 1, _pragma.literals().end());
		if (literals.empty())
			m_errorReporter.syntaxError(
				9679_error,
				_pragma.location(),
				"Experimental feature name is missing."
			);
		else if (literals.size() > 1)
			m_errorReporter.syntaxError(
				6022_error,
				_pragma.location(),
				"Stray arguments."
			);
		else
		{
			string const literal = literals[0];
			if (literal.empty())
				m_errorReporter.syntaxError(3250_error, _pragma.location(), "Empty experimental feature name is invalid.");
			else if (!ExperimentalFeatureNames.count(literal))
				m_errorReporter.syntaxError(8491_error, _pragma.location(), "Unsupported experimental feature name.");
			else if (m_sourceUnit->annotation().experimentalFeatures.count(ExperimentalFeatureNames.at(literal)))
				m_errorReporter.syntaxError(1231_error, _pragma.location(), "Duplicate experimental feature name.");
			else
			{
				auto feature = ExperimentalFeatureNames.at(literal);
				m_sourceUnit->annotation().experimentalFeatures.insert(feature);
				if (!ExperimentalFeatureWithoutWarning.count(feature))
					m_errorReporter.warning(2264_error, _pragma.location(), "Experimental features are turned on. Do not use experimental features on live deployments.");

				if (feature == ExperimentalFeature::ABIEncoderV2)
				{
					if (m_sourceUnit->annotation().useABICoderV2.set())
					{
						if (!*m_sourceUnit->annotation().useABICoderV2)
							m_errorReporter.syntaxError(
								8273_error,
								_pragma.location(),
								"ABI coder v1 has already been selected through \"pragma abicoder v1\"."
							);
					}
					else
						m_sourceUnit->annotation().useABICoderV2 = true;
				}
			}
		}
	}
	else if (_pragma.literals()[0] == "abicoder")
	{
		solAssert(m_sourceUnit, "");
		if (
			_pragma.literals().size() != 2 ||
			!set<string>{"v1", "v2"}.count(_pragma.literals()[1])
		)
			m_errorReporter.syntaxError(
				2745_error,
				_pragma.location(),
				"Expected either \"pragma abicoder v1\" or \"pragma abicoder v2\"."
			);
		else if (m_sourceUnit->annotation().useABICoderV2.set())
			m_errorReporter.syntaxError(
				3845_error,
				_pragma.location(),
				"ABI coder has already been selected for this source unit."
			);
		else
			m_sourceUnit->annotation().useABICoderV2 = (_pragma.literals()[1] == "v2");
	}
	else if ( _pragma.literals()[0] == "soliditypp" || _pragma.literals()[0] == "solidity")
	{
		vector<Token> tokens(_pragma.tokens().begin() + 1, _pragma.tokens().end());
		vector<string> literals(_pragma.literals().begin() + 1, _pragma.literals().end());
		SemVerMatchExpressionParser parser(tokens, literals);
		auto matchExpression = parser.parse();
		// An unparsable version pragma is an unrecoverable fatal error in the parser.
		solAssert(matchExpression.has_value(), "");
		static SemVerVersion const currentVersion{string(VersionString)};
		if (!matchExpression->matches(currentVersion))
			m_errorReporter.syntaxError(
				3997_error,
				_pragma.location(),
				"Source file requires different compiler version (current compiler is " +
				string(VersionString) + ") - note that nightly builds are considered to be "
				"strictly less than the released version"
			);
		m_versionPragmaFound = true;
	}
	else
		m_errorReporter.syntaxError(4936_error, _pragma.location(), "Unknown pragma \"" + _pragma.literals()[0] + "\"");

	return true;
}
