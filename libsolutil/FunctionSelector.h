// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <libsolutil/Blake2.h>
#include <libsolutil/FixedHash.h>

#include <string>

namespace solidity::util
{

/// @returns the ABI selector for a given function signature, as a 32 bit number.
inline uint32_t selectorFromSignature32(std::string const& _signature)
{
	// Solidity++: keccak256 -> blake2b
	return uint32_t(FixedHash<4>::Arith(util::FixedHash<4>(util::blake2b(_signature))));
}

/// @returns the ABI selector for a given function signature, as a u256 (left aligned) number.
inline u256 selectorFromSignature(std::string const& _signature)
{
	return u256(selectorFromSignature32(_signature)) << (256 - 32);
}


}
