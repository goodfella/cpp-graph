#ifndef UNIVERSAL_SYMBOL_REFERENCE_PROPERTY_HPP
#define UNIVERSAL_SYMBOL_REFERENCE_PROPERTY_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/property.hpp"
#include <string>
#include <tuple>

class universal_symbol_reference_property
{
    public:

    universal_symbol_reference_property();

    explicit
    universal_symbol_reference_property(CXCursor cursor);

    ngmg::cypher::property<std::string> prop;

    auto
    tuple() const
    {
        return std::tie(prop);
    }

    void
    fill(CXCursor cursor);
};

#endif
