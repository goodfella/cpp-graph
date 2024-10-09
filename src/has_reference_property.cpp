#include "has_reference_property.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

has_reference_property::has_reference_property(CXCursor cursor):
    prop {has_reference_prop_name}
{
    this->fill(cursor);
}

void
has_reference_property::fill(CXCursor cursor)
{
    CXCursor ref_cursor = clang_getCursorReferenced(cursor);
    this->prop = !(clang_Cursor_isNull(ref_cursor) == 1);
}
