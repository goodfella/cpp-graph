#include "node_expression.hpp"

ngmg::cypher::detail::node_expression_base::node_expression_base(ngmg::cypher::node_variable const * variable,
                                                                 ngmg::cypher::label const * label,
                                                                 ngmg::cypher::property_set const * property_set,
                                                                 std::set<ngmg::cypher::label> const * label_set):
    _var(variable),
    _label(label),
    _property_set(property_set),
    _label_set(label_set)
{}
