#include "memgraph/cypher/label.hpp"
#include "function_decl_def_node.hpp"

function_decl_def_node::function_decl_def_node(std::string_view label):
    _label {label}
{}

const ngmg::cypher::label &
function_decl_def_node::label() const noexcept
{
    return this->_label;
}
