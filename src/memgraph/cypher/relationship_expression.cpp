#include "node_expression.hpp"
#include "relationship_expression.hpp"

ngmg::cypher::detail::relationship_expression_base::relationship_expression_base(ngmg::cypher::label const * label,
                                                                                 ngmg::cypher::relationship_variable const * variable,
                                                                                 const ngmg::cypher::relationship_type type):
    _label(label),
    _variable(variable),
    _type(type)
{}

void
ngmg::cypher::detail::relationship_expression_base::write_start(std::ostream & stream) const
{
    stream << "-[";
    if (this->_variable)
    {
        stream << this->_variable->name();
    }
    stream.put(':');

    if (this->_label)
    {
        stream << this->_label->name();
    }
}

void
ngmg::cypher::detail::relationship_expression_base::write_end(std::ostream & stream) const
{
    stream << "]-";

    if (this->_type == ngmg::cypher::relationship_type::directed)
    {
        stream.put('>');
    }
}

ngmg::cypher::detail::relationship_expression_source_base<ngmg::cypher::node_variable>::relationship_expression_source_base(const ngmg::cypher::node_variable & src):
    _src(src.name())
{}

void
ngmg::cypher::detail::relationship_expression_source_base<ngmg::cypher::node_variable>::write_source(std::ostream & stream) const
{
    stream << '(' << this->_src << ")";
}

ngmg::cypher::detail::relationship_expression_destination_base<ngmg::cypher::node_variable>::relationship_expression_destination_base(const ngmg::cypher::node_variable & dst):
    _dst(dst.name())
{}

void
ngmg::cypher::detail::relationship_expression_destination_base<ngmg::cypher::node_variable>::write_destination(std::ostream & stream) const
{
    stream << '(' << this->_dst << ")";
}
