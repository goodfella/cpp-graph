#ifndef FUNCTION_DECL_DEF_NODE_HPP
#define FUNCTION_DECL_DEF_NODE_HPP

#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include <tuple>

class function_decl_def_node
{
    public:

    explicit
    function_decl_def_node(std::string_view label);

    location_properties location;
    name_properties names;

    auto
    tuple() const noexcept
    {
        return tuple_cat(this->location.tuple(),
                         this->names.tuple());
    }

    const ngmg::cypher::label &
    label() const noexcept;

    private:

    ngmg::cypher::label _label;
};

#endif
