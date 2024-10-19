#include "memgraph/cypher/label.hpp"
#include "node_property_names.hpp"
#include "unknown_callee_node.hpp"

unknown_callee_node::unknown_callee_node(CXCursor cursor):
    location (cursor),
    usr (cursor),
    kind (cursor)
{}

const ngmg::cypher::label &
unknown_callee_node::label() noexcept
{
    static const ngmg::cypher::label label {"UnknownCallee"};
    return label;
}
