#ifndef CALL_EXPR_NODE_HPP
#define CALL_EXPR_NODE_HPP

#include <clang-c/Index.h>
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

    auto
    tuple() const noexcept
    {
        return tuple_cat(location.tuple());
    }

    static
    const ngmg::cypher::label &
    label() noexcept;
};

#endif
