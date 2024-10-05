#include "is_template_property.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

is_template_property::is_template_property():
    prop {is_template_prop_name}
{}

void
is_template_property::fill(CXCursor cursor)
{
    this->prop = clang_getCursorKind(cursor) == CXCursor_ClassTemplate;
}
