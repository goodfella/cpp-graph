#include "memgraph/cypher/label.hpp"
#include "namespace_decl_node.hpp"
#include "node_property_names.hpp"
#include "ngclang.hpp"

const ngmg::cypher::label &
namespace_decl_node::label() noexcept
{
    static const ngmg::cypher::label label {"NamespaceDeclaration"};
    return label;
}
