#include "label.hpp"

ngmg::cypher::label::label(const std::string_view name):
    _name(name)
{}

std::string_view
ngmg::cypher::label::name() const noexcept
{
    return this->_name;
}

void
ngmg::cypher::label::name(const std::string_view value)
{
    this->_name = value;
}
