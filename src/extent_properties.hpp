#ifndef EXTENT_PROPERTIES_HPP
#define EXTENT_PROPERTIES_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/property.hpp"
#include <string>
#include <tuple>

class extent_properties
{
    public:

    extent_properties();

    extent_properties(CXCursor cursor);

    ngmg::cypher::property<int> start_line_prop;
    ngmg::cypher::property<int> end_line_prop;
    ngmg::cypher::property<int> start_column_prop;
    ngmg::cypher::property<int> end_column_prop;
    ngmg::cypher::property<std::string> start_file_prop;
    ngmg::cypher::property<std::string> end_file_prop;

    auto
    tuple() const
    {
        return std::tie(start_line_prop,
                        end_line_prop,
                        start_column_prop,
                        end_column_prop,
                        start_file_prop,
                        end_file_prop);
    }

    void
    fill(CXCursor cursor);
};

#endif
