// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ Abstract Syntax Tree.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#include <libsolidity/ast/SolidityppAST.h>

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/ast/SolidityppAST_accept.h>
#include <libsolidity/ast/TypeProvider.h>
#include <libsolutil/Blake2.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <functional>
#include <utility>

using namespace std;
using namespace solidity;
using namespace solidity::frontend;

// Solidity++:
bool Literal::looksLikeViteAddress() const
{
	if (!boost::starts_with(value(), "vite_"))
		return false;

	if (subDenomination() != SubDenomination::None)
		return false;

	return int(value().length()) == 55;
}

// Solidity++:
bool Literal::looksLikeViteTokenId() const
{
	if (!boost::starts_with(value(), "tti_"))
		return false;

	if (subDenomination() != SubDenomination::None)
		return false;

	return int(value().length()) == 28;
}

// Solidity++: check vite address checksum
bool Literal::passesViteAddressChecksum() const
{
	return util::passesViteAddressChecksum(value());
}

// Solidity++: check vite address checksum

bool Literal::passesViteTokenIdChecksum() const
{
	return util::passesViteTokenIdChecksum(value());
}

// Solidity++: get vite address hex value
ASTString Literal::getViteAddressHex() const
{
	return util::getViteAddressHex(value());
}

// Solidity++: get vite token id hex value
ASTString Literal::getViteTokenIdHex() const
{
	return util::getViteTokenIdHex(value());
}
