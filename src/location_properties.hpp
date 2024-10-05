#ifndef LOCATION_PROPERTIES_HPP
#define LOCATION_PROPERTIES_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/property.hpp"
#include <string>
#include <tuple>

class location_properties
{
    public:

    location_properties();

    ngmg::cypher::property<int> line_prop;
    ngmg::cypher::property<int> column_prop;
    ngmg::cypher::property<std::string> file_prop;

    auto
    tuple() const
    {
        return std::tie(line_prop,
                        column_prop,
                        file_prop);
    }

    void
    fill(CXCursor cursor);
};

#endif
