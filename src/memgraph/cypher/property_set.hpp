#ifndef NGMG_CYPHER_PROPERTY_SET_HPP
#define NGMG_CYPHER_PROPERTY_SET_HPP

#include <algorithm>
#include "property.hpp"
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace ngmg::cypher
{
    class property_set
    {
        private:

        using value_type = ngmg::cypher::property_variant;
        using container_type = std::vector<value_type>;

        public:

        using const_iterator = container_type::const_iterator;

        property_set() = default;

        template <class T>
        bool
        insert(const ngmg::cypher::property<T> & property);

        template <class T>
        bool
        emplace(const std::string_view name, const T& value);

        void
        clear() noexcept;

        template <class ... Args>
        void
        reset(Args && ... args);

        const_iterator
        cbegin() const noexcept;

        const_iterator
        cend() const noexcept;

        const_iterator
        begin() const noexcept;

        const_iterator
        end() const noexcept;

        bool
        empty() const noexcept;

        std::size_t
        size() const noexcept;

        private:

        container_type _container;
    };

    template <class ... Args>
    void
    fill_property_set(ngmg::cypher::property_set & set, Args ... args);

    template <class ... Args>
    property_set
    make_property_set(Args ... args);

    void
    write_property_set(std::ostream & stream, const ngmg::cypher::property_set & property_set);

    namespace detail
    {
        template <class T>
        inline
        void
        fill_property_set(ngmg::cypher::property_set & set, const ngmg::cypher::property<T> & property)
        {
            set.insert(property);
        }

        template <class T, class ... Args>
        void
        fill_property_set(ngmg::cypher::property_set & set, const ngmg::cypher::property<T> & property, Args ... args)
        {
            detail::fill_property_set(set, property);
            detail::fill_property_set(set, args...);
        }
    }

    template <class ... Args>
    void
    fill_property_set(ngmg::cypher::property_set & set, Args ... args)
    {
        detail::fill_property_set(set, args...);
    }

    template <class ... Args>
    property_set
    make_property_set(Args ... args)
    {
        ngmg::cypher::property_set set;
        detail::fill_property_set(set, args...);
        return set;
    }

    template <class T>
    bool
    property_set::insert(const ngmg::cypher::property<T> & property)
    {
        auto iter = std::find(this->_container.begin(), this->_container.end(), value_type{property});
        if (iter == this->_container.cend())
        {
            // Property is not in the set
            this->_container.push_back(property);
            return true;
        }

        if (std::holds_alternative<ngmg::cypher::property<T>>(*iter))
        {
            // property type matches so allow value re-assignment
            *iter = property;
            return false;
        }

        // property type missmatch, so don't allow insertion
        throw std::logic_error("property type missmatch");
    }

    template <class T>
    bool
    property_set::emplace(const std::string_view name, const T & value)
    {
        return this->insert(ngmg::cypher::property<T> {name, value});
    }

    template <class ... Args>
    void
    property_set::reset(Args && ... args)
    {
        this->clear();
        ngmg::cypher::fill_property_set(*this, std::forward<Args>(args)...);
    }

    template <class ... Ps>
    requires ngmg::cypher::is_property_tuple<std::tuple<Ps...>>::value
    void
    write_property_collections(std::ostream & stream,
                               const std::tuple<Ps...> & props,
                               const ngmg::cypher::property_set & prop_set)
    {
        constexpr bool tuple_props_exist = sizeof...(Ps) > 0;
        const bool prop_set_props_exist = !prop_set.empty();

        if (tuple_props_exist || prop_set_props_exist)
        {
            stream.put('{');

            if constexpr (tuple_props_exist)
            {
                ngmg::cypher::detail::write_properties(stream, props);
            }

            if (prop_set_props_exist)
            {
                if constexpr (tuple_props_exist)
                {
                    stream << ", ";
                }

                ngmg::cypher::write_property_set(stream, prop_set);
            }

            stream.put('}');
        }
    }
}

#endif
