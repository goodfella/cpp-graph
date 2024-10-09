#include "call_expr_node.hpp"
#include "memgraph/cypher/label.hpp"
#include "node_labels.hpp"
#include "node_property_names.hpp"

call_expr_node::call_expr_node(CXCursor cursor):
    location(cursor),
    has_reference(cursor),
    function_def_present(function_def_present_prop_name)
{}

const ngmg::cypher::label &
call_expr_node::label() noexcept
{
    return call_expr_label;
}
