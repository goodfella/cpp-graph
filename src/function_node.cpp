#include "memgraph/cypher/label.hpp"
#include "function_node.hpp"

function_node::function_node(std::string_view label):
    _label {label}
{}

const ngmg::cypher::label &
function_node::label() const noexcept
{
    return this->_label;
}
