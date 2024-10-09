#include "function_node.hpp"
#include "memgraph/cypher/label.hpp"
#include "node_labels.hpp"

function_node::function_node(std::string_view label):
    _label {label}
{
    this->_label_set.emplace(label);
    this->_label_set.emplace(callable_label);
}

const ngmg::cypher::label &
function_node::label() const noexcept
{
    return this->_label;
}

const ngmg::cypher::label_set &
function_node::label_set() const noexcept
{
    return this->_label_set;
}
