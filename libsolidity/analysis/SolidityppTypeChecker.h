// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ Type analyzer and checker.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#pragma once

#include <liblangutil/EVMVersion.h>

#include <libsolidity/ast/ASTAnnotations.h>
#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/ast/Types.h>
#include <libsolidity/analysis/TypeChecker.h>

namespace solidity::langutil
{
class ErrorReporter;
}

namespace solidity::frontend
{

/**
 * The module that performs type analysis on the AST, checks the applicability of operations on
 * those types and stores errors for invalid operations.
 * Provides a way to retrieve the type of an AST node.
 */
class SolidityppTypeChecker: public TypeChecker
{
public:
	/// @param _errorReporter provides the error logging functionality.
	SolidityppTypeChecker(langutil::EVMVersion _evmVersion, langutil::ErrorReporter& _errorReporter):
	    TypeChecker(_evmVersion, _errorReporter)
	{}

protected:
    using TypeChecker::visit;
	using TypeChecker::endVisit;

	bool visit(SourceUnit const& _sourceUnit) override;
    bool visit(AwaitExpression const& _awaitExpression) override;
    bool visit(FunctionDefinition const& _function) override;
    bool visit(FunctionCall const& _functionCall) override;
	bool visit(FunctionCallOptions const& _functionCallOptions) override;
	void endVisit(Literal const& _literal) override;

	SourceLanguage m_sourceLanguage;

};

}
