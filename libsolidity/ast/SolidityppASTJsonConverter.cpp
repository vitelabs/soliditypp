// SPDX-License-Identifier: GPL-3.0
#include <libsolidity/ast/SolidityppASTJsonConverter.h>

#include <libsolidity/ast/TypeProvider.h>

#include <libyul/AsmPrinter.h>
#include <libyul/AST.h>
#include <libyul/backends/evm/EVMDialect.h>

#include <libsolutil/JSON.h>

#include <utility>
#include <vector>
#include <type_traits>

using namespace std;
using namespace solidity::langutil;

namespace solidity::frontend
{

    SolidityppASTJsonConverter::SolidityppASTJsonConverter(CompilerStack::State _stackState, map<string, unsigned> _sourceIndices):
    ASTJsonConverter(_stackState, _sourceIndices)
{
}

bool SolidityppASTJsonConverter::visit(FunctionCall const& _node)
{
	Json::Value names(Json::arrayValue);
	for (auto const& name: _node.names())
		names.append(Json::Value(*name));
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("expression", toJson(_node.expression())),
		make_pair("names", std::move(names)),
		make_pair("arguments", toJson(_node.arguments())),
		make_pair("tryCall", _node.annotation().tryCall),
		make_pair("async", _node.annotation().async)  // Solidity++: synchrony of the function call
	};

	if (_node.annotation().kind.set())
	{
		FunctionCallKind nodeKind = *_node.annotation().kind;
		attributes.emplace_back("kind", functionCallKind(nodeKind));
	}

	appendExpressionAttributes(attributes, _node.annotation());
	setJsonNode(_node, "FunctionCall", std::move(attributes));
	return false;
}

bool SolidityppASTJsonConverter::visit(AwaitExpression const& _node)
{
    std::vector<pair<string, Json::Value>> attributes = {
            make_pair("expression", toJson(_node.expression())),
    };
    setJsonNode(_node, "AwaitExpression", std::move(attributes));
    return false;
}
}
