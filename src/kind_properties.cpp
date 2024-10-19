#include "kind_properties.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

kind_properties::kind_properties():
    kind_prop {kind_prop_name},
    kind_spelling_prop {kind_spelling_prop_name}
{}

kind_properties::kind_properties(CXCursor cursor):
    kind_properties ()
{
    this->fill(cursor);
}

void
kind_properties::fill(CXCursor cursor)
{
    const CXCursorKind kind = clang_getCursorKind(cursor);
    this->kind_prop = kind;

    const ngclang::string_t kind_spelling = clang_getCursorKindSpelling(kind);
    this->kind_spelling_prop = ngclang::to_string(kind_spelling.get());
}
