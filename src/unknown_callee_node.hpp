#ifndef UNKNOWN_CALLEE_NODE_HPP
#define UNKNOWN_CALLEE_NODE_HPP

#include <clang-c/Index.h>
#include "kind_properties.hpp"
#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include "memgraph/cypher/property_set.hpp"
#include <tuple>
#include "universal_symbol_reference_property.hpp"

class unknown_callee_node
{
    public:

    unknown_callee_node (CXCursor cursor);

    location_properties location;
    universal_symbol_reference_property usr;
    kind_properties kind;
    ngmg::cypher::property_set property_set;

    auto
    tuple() const noexcept
    {
        return tuple_cat(location.tuple(),
                         usr.tuple(),
                         kind.tuple());
    }

    static
    const ngmg::cypher::label &
    label() noexcept;
};

#endif
