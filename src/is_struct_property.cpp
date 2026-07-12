#include "is_struct_property.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

is_struct_property::is_struct_property():
    prop {is_struct_prop_name}
{}

void
is_struct_property::fill(CXCursor cursor)
{
    const CXCursorKind kind = clang_getCursorKind(cursor);
    this->prop =
        (kind == CXCursor_StructDecl);
}
