#include "namespace_node.hpp"

const ngmg::cypher::label &
namespace_node::label() noexcept
{
    static const ngmg::cypher::label l ("Namespace");
    return l;
}
