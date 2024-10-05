#ifndef CLASS_DECL_NODE_HPP
#define CLASS_DECL_NODE_HPP

#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include <tuple>

class class_decl_node
{
    public:

    location_properties location;
    name_properties names;

    auto
    tuple() const noexcept
    {
        return tuple_cat(this->location.tuple(),
                         this->names.tuple());
    }

    static
    const ngmg::cypher::label &
    label() noexcept;
};

#endif
