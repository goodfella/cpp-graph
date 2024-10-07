#include "name_properties.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"
#include <string>

name_properties::name_properties():
    name_prop {name_prop_name},
    fq_name_prop {fq_name_prop_name},
    unqualified_name_prop {unqualified_name_prop_name}
{}

void
name_properties::fill_with_fq_name(CXCursor cursor, std::string_view fq_name)
{
    this->fq_name_prop = fq_name;
    this->name_prop = ngclang::to_string(cursor, &clang_getCursorDisplayName);
    this->unqualified_name_prop = ngclang::to_string(cursor, &clang_getCursorSpelling);
}

void
name_properties::fill_with_fq_namespace(CXCursor cursor, std::string_view fq_namespace)
{
    this->unqualified_name_prop = ngclang::to_string(cursor, &clang_getCursorSpelling);
    this->name_prop = ngclang::to_string(cursor, &clang_getCursorDisplayName);
    const std::string fq_name = [fq_namespace, this] () {
        std::string t {fq_namespace};
        t += "::";
        t += this->unqualified_name_prop.value();
        return t;
    }();

    this->fq_name_prop = fq_name;
}
