#include "variable.hpp"

extern template class ngmg::cypher::variable<ngmg::cypher::variable_tag::node>;
extern template class ngmg::cypher::variable<ngmg::cypher::variable_tag::relationship>;

ngmg::cypher::detail::variable_base::variable_base(const std::string_view name):
    _name(name)
{}

std::string_view
ngmg::cypher::detail::variable_base::name() const noexcept
{
    return this->_name;
}
