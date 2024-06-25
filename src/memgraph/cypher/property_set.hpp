#ifndef NGMG_CYPHER_PROPERTY_SET_HPP
#define NGMG_CYPHER_PROPERTY_SET_HPP

#include <algorithm>
#include "property.hpp"
#include <stdexcept>
#include <vector>

namespace ngmg::cypher
{
    class property_set
    {
        private:

        using value_type =
            std::variant<ngmg::cypher::property<int>,
                         ngmg::cypher::property<std::string>>;

        using container_type = std::vector<value_type>;

        public:

        using const_iterator = container_type::const_iterator;

        property_set() = default;

        template <class T>
        bool
        insert(const ngmg::cypher::property<T> & property);

        void
        clear() noexcept;

        template <class ... Args>
        void
        reset(Args ... args);

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

    template <class ... Args>
    void
    write_properties(std::ostream & stream, Args ... args);

    namespace detail
    {
        void
        write_property_set(std::ostream & stream, const ngmg::cypher::property_set & property_set);

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

    template <class ... Args>
    void
    property_set::reset(Args ... args)
    {
        this->clear();
        ngmg::cypher::fill_property_set(*this, args...);
    }

    namespace detail
    {
        template <class ... Args>
        void
        write_properties(std::ostream & stream, const ngmg::cypher::property_set & set, Args ... args)
        {
            ngmg::cypher::detail::write_property_set(stream, set);

            if constexpr (sizeof...(args) > 0)
            {
                stream.write(", ", 2U);
                ngmg::cypher::detail::write_properties(stream, args...);
            }
        }

        template <class T, class ... Args>
        void
        write_properties(std::ostream & stream, const ngmg::cypher::property<T> property, Args ... args)
        {
            property.write(stream);

            if constexpr (sizeof...(args) > 0)
            {
                stream.write(", ", 2U);
                ngmg::cypher::detail::write_properties(stream, args...);
            }
        }
    }

    template <class ... Args>
    void
    write_properties(std::ostream & stream, Args ... args)
    {
        stream.put('{');
        ngmg::cypher::detail::write_properties(stream, args...);
        stream.put('}');
    }
}

#endif
