#include "variable.hpp"

ngmg::cypher::variable::variable(const std::string_view name):
    _name(name)
{}

std::string_view
ngmg::cypher::variable::name() const noexcept
{
    return this->_name;
}
