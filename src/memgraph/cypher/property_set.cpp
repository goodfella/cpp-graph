#include "property_set.hpp"

void
ngmg::cypher::property_set::clear() noexcept
{
    this->_container.clear();
}

ngmg::cypher::property_set::const_iterator
ngmg::cypher::property_set::cbegin() const noexcept
{
    return this->_container.cbegin();
}


ngmg::cypher::property_set::const_iterator
ngmg::cypher::property_set::cend() const noexcept
{
    return this->_container.cend();
}

ngmg::cypher::property_set::const_iterator
ngmg::cypher::property_set::begin() const noexcept
{
    return this->_container.cbegin();
}


ngmg::cypher::property_set::const_iterator
ngmg::cypher::property_set::end() const noexcept
{
    return this->_container.cend();
}

bool
ngmg::cypher::property_set::empty() const noexcept
{
    return this->_container.empty();
}

std::size_t
ngmg::cypher::property_set::size() const noexcept
{
    return this->_container.size();
}

void
ngmg::cypher::write_property_set(std::ostream & stream, const ngmg::cypher::property_set & set)
{
    if (set.empty())
    {
        return;
    }

    {
        auto it = set.cbegin();
        auto visitor = [&stream](auto && p)
        {
            p.write(stream);
        };

        std::visit(visitor, *it);
        ++it;

        for (;it != set.cend(); ++it)
        {
            stream << ", ";
            std::visit(visitor, *it);
        }
    }
}
