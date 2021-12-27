// SPDX-License-Identifier: GPL-3.0
/**
 * @author Charles <charles@vite.org>
 * @date 2021
 * Solidity++ Abstract Syntax Tree.
 * Solidity++ is modified from Solidity under the terms of the GNU General Public License.
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/ast/Types.h>
#include <libsolidity/ast/ASTAnnotations.h>
#include <libsolidity/ast/ASTEnums.h>
#include <libsolidity/parsing/Token.h>

#include <liblangutil/SourceLocation.h>
#include <libevmasm/Instruction.h>
#include <libsolutil/FixedHash.h>
#include <libsolutil/LazyInit.h>

#include <boost/noncopyable.hpp>
#include <json/json.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace solidity::frontend
{

class ASTVisitor;
class ASTConstVisitor;

/**
 * Solidity++:
 * The await expression is used to wait for an asynchronous message call to return: await foo(arg1, ..., argn)
 */
class AwaitExpression: public Expression
{
public:
    AwaitExpression(
            int64_t _id,
            SourceLocation const& _location,
            ASTPointer<Expression> _expression
    ):
            Expression(_id, _location), m_expression(std::move(_expression)) {}
    void accept(ASTVisitor& _visitor) override;
    void accept(ASTConstVisitor& _visitor) const override;

    Expression const& expression() const { return *m_expression; }

private:
    ASTPointer<Expression> m_expression;
};

}
