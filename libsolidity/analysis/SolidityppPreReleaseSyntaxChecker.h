// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ syntax checker for pre-release versions
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#pragma once

#include <libsolidity/analysis/SolidityppTypeChecker.h>
#include <libsolidity/ast/Types.h>
#include <libsolidity/ast/ASTAnnotations.h>
#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/ast/ASTVisitor.h>

namespace solidity::frontend
{

/**
 * The module that performs syntax analysis for Solidity++
 */
class SolidityppPreReleaseSyntaxChecker: public ASTConstVisitor
{
public:
    /// @param _errorReporter provides the error logging functionality.
    SyntaxChecker(langutil::ErrorReporter& _errorReporter):
    m_errorReporter(_errorReporter)
    {}

    bool checkSyntax(ASTNode const& _astRoot);

protected:
	langutil::ErrorReporter& m_errorReporter;
};

}
