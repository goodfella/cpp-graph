#ifndef CLASS_NODE_HPP
#define CLASS_NODE_HPP

#include "is_template_property.hpp"
#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include <tuple>
#include "universal_symbol_reference_property.hpp"

class class_node
{
    public:

    name_properties names;
    universal_symbol_reference_property usr;
    is_template_property is_template;

    auto
    tuple() const noexcept
    {
        return tuple_cat(names.tuple(),
                         usr.tuple(),
                         is_template.tuple());
    }

    static
    const ngmg::cypher::label &
    label() noexcept;
};

#endif
