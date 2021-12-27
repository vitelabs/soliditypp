/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
 * @author Christian <c@ethdev.com>
 * @date 2015
 * Versioning.
 */

#pragma once

#include <libsolutil/Common.h>
#include <string>

namespace solidity::frontend
{

extern char const* VersionNumber;
extern std::string const VersionString;
extern std::string const VersionStringStrict;
extern bytes const VersionCompactBytes;
extern bool const VersionIsRelease;
// Solidity++:
extern char const* SolidityppVersionNumber;
extern std::string const SolidityppVersionString;
extern std::string const SolidityppVersionStringStrict;
extern bytes const SolidityppVersionCompactBytes;
extern bool const SolidityppVersionIsRelease;

}
