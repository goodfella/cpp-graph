#include "memgraph/cypher/label.hpp"
#include "node_labels.hpp"
#include "class_def_node.hpp"

const ngmg::cypher::label &
class_def_node::label() noexcept
{
    return class_def_label;
}
