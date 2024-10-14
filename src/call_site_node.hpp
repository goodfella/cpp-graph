#ifndef CALL_SITE_NODE_HPP
#define CALL_SITE_NODE_HPP

#include <clang-c/Index.h>
#include "has_reference_property.hpp"
#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include "name_properties.hpp"
#include <tuple>

class call_site_node
{
    public:

    call_site_node () = default;

    explicit
    call_site_node (CXCursor cursor);

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
