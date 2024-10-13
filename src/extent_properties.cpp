#include "extent_properties.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

extent_properties::extent_properties():
    start_line_prop {start_line_prop_name},
    end_line_prop {end_line_prop_name},
    start_column_prop {start_column_prop_name},
    end_column_prop {end_column_prop_name},
    start_file_prop {start_file_prop_name},
    end_file_prop {end_file_prop_name}
{}

extent_properties::extent_properties(CXCursor cursor):
    extent_properties ()
{
    this->fill(cursor);
}

void
extent_properties::fill(CXCursor cursor)
{
    const ngclang::cursor_range range {cursor};

    this->start_line_prop = range.start_line();
    this->start_column_prop = range.start_column();
    this->start_file_prop = range.start_file();

    this->end_line_prop = range.end_line();
    this->end_column_prop = range.end_column();
    this->end_file_prop = range.end_file();
}
