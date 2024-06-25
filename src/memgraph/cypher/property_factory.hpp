#ifndef NGMG_CYPHER_PROPERTY_FACTORY_HPP
#define NGMG_CYPHER_PROPERTY_FACTORY_HPP

#include "property_cache.hpp"
#include <string_view>

namespace ngmg::cypher
{
    class property_factory
    {
        public:

        property_factory() = default;

        property_factory(const property_factory &) = delete;
        property_factory & operator = (const property_factory &) = delete;

        property_factory(property_factory &&) = default;
        property_factory & operator = (property_factory &&) = default;

        template <class T>
        ngmg::cypher::property<T>
        make_property(std::string_view name, const T v);

        private:

        ngmg::cypher::property_cache _cache;
    };

    template <class T>
    ngmg::cypher::property<T>
    property_factory::make_property(std::string_view name, const T v)
    {
        return ngmg::cypher::property {name, v, this->_cache};
    }
}

#endif
