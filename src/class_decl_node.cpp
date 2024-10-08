#include "memgraph/cypher/label.hpp"
#include "class_decl_node.hpp"

const ngmg::cypher::label &
class_decl_node::label() noexcept
{
    static const ngmg::cypher::label label {"ClassDeclaration"};
    return label;
}
