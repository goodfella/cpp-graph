#ifndef IS_TEMPLATE_PROPERTY_HPP
#define IS_TEMPLATE_PROPERTY_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/property.hpp"
#include <string>
#include <tuple>

class is_template_property
{
    public:

    is_template_property();

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
