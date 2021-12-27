// SPDX-License-Identifier: GPL-3.0
#pragma once

#include <libsolidity/ast/ASTAnnotations.h>
#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/interface/CompilerStack.h>
#include <liblangutil/Exceptions.h>

#include <json/json.h>

#include <algorithm>
#include <optional>
#include <ostream>
#include <stack>
#include <vector>
#include <libsolidity/ast/ASTJsonConverter.h>

namespace solidity::langutil
{
struct SourceLocation;
}

namespace solidity::frontend
{

class SolidityppASTJsonConverter: public ASTJsonConverter
{
public:
	explicit SolidityppASTJsonConverter(
		CompilerStack::State _stackState,
		std::map<std::string, unsigned> _sourceIndices = std::map<std::string, unsigned>()
	);

	using ASTJsonConverter::visit;

	bool visit(FunctionCall const& _node) override;
    bool visit(AwaitExpression const& _node) override;
};

}
