#ifndef NAMESPACE_DECL_NODE_HPP
#define NAMESPACE_DECL_NODE_HPP

#include <clang-c/Index.h>
#include "location_properties.hpp"
#include "memgraph/cypher/label.hpp"
#include "memgraph/cypher/property.hpp"
#include "name_properties.hpp"
#include <string_view>
#include <tuple>

class namespace_decl_node
{
    public:

    namespace_decl_node() = default;

    location_properties location;
    name_properties names;

    auto
    tuple() const
    {
        return
            std::tuple_cat(this->location.tuple(),
                           this->names.tuple());
    }

    static
    const ngmg::cypher::label &
    label() noexcept;
};

#endif
