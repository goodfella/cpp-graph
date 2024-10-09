#include "memgraph/cypher/label.hpp"
#include "function_decl_def_node.hpp"

function_decl_def_node::function_decl_def_node(std::string_view label, callable_node_type node_type):
    _label {label}
{
    this->_label_set.emplace(label);

    if (node_type == callable_node_type::callable_declaration)
    {
        this->_label_set.emplace("CallableDeclaration");
    }
    else if (node_type == callable_node_type::callable_definition)
    {
        this->_label_set.emplace("CallableDefinition");
    }
}

const ngmg::cypher::label &
function_decl_def_node::label() const noexcept
{
    return this->_label;
}

const ngmg::cypher::label_set &
function_decl_def_node::label_set() const noexcept
{
    return this->_label_set;
}
