#include "relationship_expression.hpp"

ngmg::cypher::detail::relationship_expression_base::relationship_expression_base(const ngmg::cypher::variable & src,
                                                                                 const ngmg::cypher::label & label,
                                                                                 const ngmg::cypher::variable & dst,
                                                                                 const ngmg::cypher::relationship_type type):
    _src(src.name()),
    _dst(dst.name()),
    _label(label.name()),
    _type(type)
{}
