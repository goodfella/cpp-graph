#ifndef NAMESPACE_NODE_HPP
#define NAMESPACE_NODE_HPP

#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include "universal_symbol_reference_property.hpp"
#include <tuple>

class namespace_node
{
    public:

    namespace_node() = default;

    name_properties names;
    universal_symbol_reference_property usr;

    static
    const ngmg::cypher::label &
    label() noexcept;

    auto
    tuple()
    {
        return std::tuple_cat(names.tuple(), std::tie(usr.prop));
    }
};

#endif
