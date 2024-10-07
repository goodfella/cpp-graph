#include "is_template_property.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

is_template_property::is_template_property():
    prop {is_template_prop_name}
{}

void
is_template_property::fill(CXCursor cursor)
{
    const CXCursorKind kind = clang_getCursorKind(cursor);
    this->prop =
        (kind == CXCursor_ClassTemplate ||
         kind == CXCursor_FunctionTemplate ||
         kind == CXCursor_ClassTemplatePartialSpecialization);
}
