#ifndef IS_STRUCT_PROPERTY_HPP
#define IS_STRUCT_PROPERTY_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/property.hpp"
#include <string>
#include <tuple>

class is_struct_property
{
    public:

    is_struct_property();

    ngmg::cypher::property<bool> prop;

    auto
    tuple() const
    {
        return std::tie(prop);
    }

    void
    fill(CXCursor cursor);
};

#endif
