#ifndef NGMG_CYPHER_PROPERTY_CACHE_HPP
#define NGMG_CYPHER_PROPERTY_CACHE_HPP

#include <map>
#include <string>
#include <string_view>
#include <variant>

namespace ngmg::cypher
{
    class property_cache
    {
        public:

        using value_type =
            std::variant<int,
                         std::string>;

        private:
        using container_type = std::map<std::string, value_type, std::less<>>;

        public:

        using iterator = container_type::iterator;
        using const_iterator = container_type::const_iterator;

        property_cache() = default;

        property_cache(const property_cache &) = delete;
        property_cache & operator= (const property_cache &) = delete;

        property_cache(property_cache &&) = default;
        property_cache & operator= (property_cache &&) = default;

        iterator
        insert(const std::string_view name, const value_type value);

        const_iterator
        find(const std::string_view name) const noexcept;

        private:

        container_type _map;
    };
}

#endif
