#ifndef KIND_PROPERTIES_HPP
#define KIND_PROPERTIES_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/property.hpp"
#include <string>
#include <tuple>

class kind_properties
{
    public:

    kind_properties();

    kind_properties(CXCursor cursor);

    ngmg::cypher::property<int> kind_prop;
    ngmg::cypher::property<std::string> kind_spelling_prop;

    auto
    tuple() const
    {
        return std::tie(kind_prop,
                        kind_spelling_prop);
    }

    void
    fill(CXCursor cursor);
};

#endif
