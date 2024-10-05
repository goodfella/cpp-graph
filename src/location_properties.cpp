#include "location_properties.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

location_properties::location_properties():
    line_prop {line_prop_name},
    column_prop {column_prop_name},
    file_prop {file_prop_name}
{}

void
location_properties::fill(CXCursor cursor)
{
    const ngclang::cursor_location location {cursor};

    this->line_prop = location.line();
    this->column_prop = location.column();
    this->file_prop = location.file();
}
