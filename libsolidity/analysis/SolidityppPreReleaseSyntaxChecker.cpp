// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ syntax checker for pre-release versions
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#include <libsolidity/analysis/SolidityppPreReleaseSyntaxChecker.h>

#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/ExperimentalFeatures.h>
#include <libsolidity/interface/Version.h>

#include <libyul/AST.h>

#include <liblangutil/ErrorReporter.h>
#include <liblangutil/SemVerHandler.h>


using namespace std;
using namespace solidity;
using namespace solidity::langutil;
using namespace solidity::frontend;
using namespace solidity::util;

bool SolidityppPreReleaseSyntaxChecker::checkSyntax(ASTNode const& _astRoot)
{
    _astRoot.accept(*this);
    return Error::containsOnlyWarnings(m_errorReporter.errors());
}

