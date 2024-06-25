#include "property_cache.hpp"
#include <stdexcept>
#include <utility>

ngmg::cypher::property_cache::iterator
ngmg::cypher::property_cache::insert(const std::string_view name, const ngmg::cypher::property_cache::value_type value)
{
    iterator iter = this->_map.find(name);
    if (iter == this->_map.end())
    {
        // value is not in cache, so insert it
        auto const pair = this->_map.insert(std::make_pair(std::string{name}, value));
        return pair.first;
    }

    if (value.index() == iter->second.index())
    {
        // types are the same so allow value re-assignment
        iter->second = value;
        return iter;
    }

    // value is already in the cache and is being assigned a different type
    throw std::logic_error("missmatch type");
}

ngmg::cypher::property_cache::const_iterator
ngmg::cypher::property_cache::find(const std::string_view name) const noexcept
{
    return this->_map.find(name);
}
