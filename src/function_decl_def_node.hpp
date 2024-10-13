#ifndef FUNCTION_DECL_DEF_NODE_HPP
#define FUNCTION_DECL_DEF_NODE_HPP

#include "extent_properties.hpp"
#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include "node_types.hpp"
#include <tuple>

enum class callable_node_type
{
    callable_definition = 1,
    callable_declaration
};

class function_decl_def_node
{
    public:

    explicit
    function_decl_def_node(std::string_view label, callable_node_type node_type);

    location_properties location;
    name_properties names;
    extent_properties extent;

    auto
    tuple() const noexcept
    {
        return tuple_cat(this->location.tuple(),
                         this->names.tuple(),
                         extent.tuple());
    }

    const ngmg::cypher::label &
    label() const noexcept;

    const ngmg::cypher::label_set &
    label_set() const noexcept;

    private:

    ngmg::cypher::label _label;
    ngmg::cypher::label_set _label_set;
};

#endif
