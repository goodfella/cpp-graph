#include "name_properties.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

name_properties::name_properties():
    name_prop {name_prop_name},
    fq_name_prop {fq_name_prop_name},
    unqualified_name_prop {unqualified_name_prop_name}
{}

void
name_properties::fill(CXCursor cursor, std::string_view fq_name)
{
    this->fq_name_prop = fq_name;
    this->name_prop = ngclang::to_string(cursor, &clang_getCursorDisplayName);
    this->unqualified_name_prop = ngclang::to_string(cursor, &clang_getCursorSpelling);
}
