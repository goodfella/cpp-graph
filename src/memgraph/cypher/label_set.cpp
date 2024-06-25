#include <algorithm>
#include "label_set.hpp"

bool
ngmg::cypher::label_set::insert(std::string_view label)
{
    if (std::find(this->_container.cbegin(), this->_container.cend(), label) == this->_container.cend())
    {
        this->_container.push_back(ngmg::cypher::label(label));
        return true;
    }

    return false;
}

ngmg::cypher::label_set::const_iterator
ngmg::cypher::label_set::cbegin() const noexcept
{
    return this->_container.cbegin();
}


ngmg::cypher::label_set::const_iterator
ngmg::cypher::label_set::cend() const noexcept
{
    return this->_container.cend();
}

bool
ngmg::cypher::label_set::empty() const noexcept
{
    return this->_container.empty();
}

std::size_t
ngmg::cypher::label_set::size() const noexcept
{
    return this->_container.size();
}
