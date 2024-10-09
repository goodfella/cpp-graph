#include "memgraph/cypher/label.hpp"
#include "call_expr_node.hpp"

call_expr_node::call_expr_node(CXCursor cursor):
    location(cursor)
{}

const ngmg::cypher::label &
call_expr_node::label() noexcept
{
    static const ngmg::cypher::label label {"CallExpr"};
    return label;
}
