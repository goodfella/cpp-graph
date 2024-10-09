#ifndef HAS_REFERENCE_PROPERTY_HPP
#define HAS_REFERENCE_PROPERTY_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/property.hpp"
#include <string>
#include <tuple>

class has_reference_property
{
    public:

    has_reference_property(CXCursor cursor);

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
