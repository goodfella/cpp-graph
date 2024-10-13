#ifndef CLASS_DEF_NODE_HPP
#define CLASS_DEF_NODE_HPP

#include "extent_properties.hpp"
#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include <tuple>

class class_def_node
{
    public:

    location_properties location;
    name_properties names;
    extent_properties extent;

    auto
    tuple() const noexcept
    {
        return tuple_cat(this->location.tuple(),
                         this->names.tuple(),
                         this->extent.tuple());
    }

    static
    const ngmg::cypher::label &
    label() noexcept;
};

#endif
