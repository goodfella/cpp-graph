#ifndef FUNCTION_NODE_HPP
#define FUNCTION_NODE_HPP

#include "is_template_property.hpp"
#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include <string_view>
#include <tuple>
#include "universal_symbol_reference_property.hpp"

class function_node
{
    public:

    explicit
    function_node(std::string_view label);

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

    const ngmg::cypher::label &
    label() const noexcept;

    private:

    ngmg::cypher::label _label;
};

#endif
