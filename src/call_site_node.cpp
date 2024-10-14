#include "call_site_node.hpp"
#include "memgraph/cypher/label.hpp"
#include "node_labels.hpp"
#include "node_property_names.hpp"

call_site_node::call_site_node(CXCursor cursor):
    location(cursor)
{}

const ngmg::cypher::label &
call_site_node::label() noexcept
{
    return call_site_label;
}
