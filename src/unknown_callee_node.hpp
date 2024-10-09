#ifndef UNKNOWN_CALLEE_NODE_HPP
#define UNKNOWN_CALLEE_NODE_HPP

#include <clang-c/Index.h>
#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include <tuple>
#include "universal_symbol_reference_property.hpp"

class unknown_callee_node
{
    public:

    unknown_callee_node (CXCursor cursor);

    location_properties location;
    universal_symbol_reference_property usr;

    auto
    tuple() const noexcept
    {
        return tuple_cat(location.tuple(),
                         usr.tuple());
    }

    static
    const ngmg::cypher::label &
    label() noexcept;
};

#endif
