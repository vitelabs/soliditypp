// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ syntax checker
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#pragma once

#include <libsolidity/analysis/SolidityppTypeChecker.h>
#include <libsolidity/analysis/SyntaxChecker.h>
#include <libsolidity/ast/Types.h>
#include <libsolidity/ast/ASTAnnotations.h>
#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/ast/ASTVisitor.h>

namespace solidity::langutil
{
class ErrorReporter;
}

namespace solidity::frontend
{

/**
 * The module that performs syntax analysis for Solidity++
 */
class SolidityppSyntaxChecker: public SyntaxChecker
{
public:
	/// @param _errorReporter provides the error logging functionality.
	SolidityppSyntaxChecker(langutil::ErrorReporter& _errorReporter, bool _useYulOptimizer):
	    SyntaxChecker(_errorReporter, _useYulOptimizer)
	{}

protected:
	void endVisit(SourceUnit const& _sourceUnit) override;
	bool visit(PragmaDirective const& _pragma) override;
};

}
