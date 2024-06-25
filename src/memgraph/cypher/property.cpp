#include "property.hpp"

ngmg::cypher::property_base::property_base(std::string_view name):
    _name(name)
{}

std::string_view
ngmg::cypher::property_base::name() const noexcept
{
        return this->_name;
}

std::strong_ordering
ngmg::cypher::property_base::operator <=> (const ngmg::cypher::property_base & rhs) const
{
    return this->name() <=> rhs.name();
}

ngmg::cypher::property<std::string>::property(std::string_view name,
                                              std::string_view value):
    ngmg::cypher::property_base(name)
{}

ngmg::cypher::property<std::string>::property(std::string_view name):
    ngmg::cypher::property_base(name)
{}

const std::string &
ngmg::cypher::property<std::string>::value() const noexcept
{
    return this->_value;
}

void
ngmg::cypher::property<std::string>::value(std::string_view value)
{
    this->_value = value;
}

void
ngmg::cypher::property<std::string>::write(std::ostream & stream) const
{
        stream << this->name() << ": ";
        ngmg::cypher::detail::write_value(stream, this->value());
}

extern template class ngmg::cypher::property<int>;
