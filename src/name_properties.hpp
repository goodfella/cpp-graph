#ifndef NAME_PROPERTIES_HPP
#define NAME_PROPERTIES_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/property.hpp"
#include <string>
#include <string_view>
#include <tuple>

class name_properties
{
    public:

    name_properties();

    ngmg::cypher::property<std::string> name_prop;
    ngmg::cypher::property<std::string> fq_name_prop;
    ngmg::cypher::property<std::string> unqualified_name_prop;

    auto
    tuple() const
    {
        return std::tie(name_prop,
                        fq_name_prop,
                        unqualified_name_prop);
    }

    void
    fill_with_fq_name(CXCursor cursor, std::string_view fq_name);

    void
    fill_with_fq_namespace(CXCursor cursor, std::string_view fq_namespace);
};

#endif
