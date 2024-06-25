#include "node_expression.hpp"

ngmg::cypher::detail::node_expression_base::node_expression_base(ngmg::cypher::variable const * variable,
                                                                 ngmg::cypher::label const * label):
    _var(variable),
    _label(label)
{}
