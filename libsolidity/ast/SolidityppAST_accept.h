// SPDX-License-Identifier: GPL-3.0
#pragma once

#include <libsolidity/ast/SolidityppAST.h>
#include <libsolidity/ast/ASTVisitor.h>

namespace solidity::frontend
{
    void AwaitExpression::accept(ASTVisitor& _visitor)
    {
        if (_visitor.visit(*this))
        {
            m_expression->accept(_visitor);
        }
        _visitor.endVisit(*this);
    }

    void AwaitExpression::accept(ASTConstVisitor& _visitor) const
    {
        if (_visitor.visit(*this))
        {
            m_expression->accept(_visitor);
        }
        _visitor.endVisit(*this);
    }
}
