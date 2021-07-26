// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Enums for AST classes.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */
#pragma once

#include <liblangutil/Exceptions.h>
#include <libsolidity/ast/ASTForward.h>

#include <string>

namespace solidity::frontend
{

/// Possible lookups for function resolving
enum class VirtualLookup { Static, Virtual, Super };

/// How a function can mutate the EVM state.
enum class StateMutability { Pure, View, NonPayable, Payable };

/// Visibility ordered from restricted to unrestricted.
enum class Visibility { Default, Private, Internal, Public, External, Offchain };

/// Solidity++: How a function will be executed in VM, synchronous or asynchronous
enum class ExecutionBehavior {Sync, Async};

inline std::string executionBehaviorToString(ExecutionBehavior const& _executionBehavior)
{
    switch (_executionBehavior)
    {
        case ExecutionBehavior::Sync:
            return "sync";
        case ExecutionBehavior::Async:
            return "async";
        default:
            solAssert(false, "Unknown execution behavior.");
    }
}


enum class Arithmetic { Checked, Wrapping };

inline std::string stateMutabilityToString(StateMutability const& _stateMutability)
{
	switch (_stateMutability)
	{
	case StateMutability::Pure:
		return "pure";
	case StateMutability::View:
		return "view";
	case StateMutability::NonPayable:
		return "nonpayable";
	case StateMutability::Payable:
		return "payable";
	default:
		solAssert(false, "Unknown state mutability.");
	}
}

class Type;

/// Container for function call parameter types & names
struct FuncCallArguments
{
	/// Types of arguments
	std::vector<Type const*> types;
	/// Names of the arguments if given, otherwise unset
	std::vector<ASTPointer<ASTString>> names;

	size_t numArguments() const { return types.size(); }
	size_t numNames() const { return names.size(); }
	bool hasNamedArguments() const { return !names.empty(); }
};

enum class ContractKind { Interface, Contract, Library };

}
