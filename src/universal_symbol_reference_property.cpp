#include "universal_symbol_reference_property.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"

universal_symbol_reference_property::universal_symbol_reference_property():
    prop {usr_prop_name}
{}

universal_symbol_reference_property::universal_symbol_reference_property(CXCursor cursor):
    universal_symbol_reference_property ()
{
    this->fill(cursor);
}

void
universal_symbol_reference_property::fill(CXCursor cursor)
{
    const ngclang::universal_symbol_reference usr {cursor};

    this->prop = usr.string();
}
