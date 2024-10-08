#include "memgraph/cypher/label.hpp"
#include "class_node.hpp"

const ngmg::cypher::label &
class_node::label() noexcept
{
    static const ngmg::cypher::label label {"Class"};
    return label;
}
