#ifndef CALL_EXPR_NODE_HPP
#define CALL_EXPR_NODE_HPP

#include <clang-c/Index.h>
#include "has_reference_property.hpp"
#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include <tuple>

class call_expr_node
{
    public:

    call_expr_node () = default;

    explicit
    call_expr_node (CXCursor cursor);

    location_properties location;
    has_reference_property has_reference;
    ngmg::cypher::property<bool> function_def_present;

    auto
    tuple() const noexcept
    {
        return tuple_cat(location.tuple(),
                         has_reference.tuple(),
                         std::tie(function_def_present));
    }

    static
    const ngmg::cypher::label &
    label() noexcept;
};

#endif
