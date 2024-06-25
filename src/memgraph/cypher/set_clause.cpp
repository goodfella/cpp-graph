#include "set_clause.hpp"

ngmg::cypher::detail::set_clause_base::set_clause_base(const std::string_view variable):
    _variable(variable)
{}
