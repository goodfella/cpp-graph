#include "return_clause.hpp"

ngmg::cypher::detail::return_clause_base::return_clause_base(const std::string_view variable):
    _variable(variable)
{}
