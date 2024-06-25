#ifndef NGMG_CYPHER_PROPERTY_HPP
#define NGMG_CYPHER_PROPERTY_HPP

#include <compare>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace ngmg::cypher
{
    class property_base
    {
        public:

        property_base(std::string_view name);

        std::string_view
        name() const noexcept;

        std::strong_ordering operator <=> (const property_base &) const;
        bool operator == (const property_base &) const = default;

        private:

        std::string_view _name;
    };

    template <class T>
    concept memgraph_data_type =
        std::disjunction_v<std::is_same<T, std::string>,
                           std::is_same<T, bool>,
                           std::is_same<T, int>,
                           std::is_same<T, float>>;

    template <memgraph_data_type T>
    class property: public ngmg::cypher::property_base
    {
        public:

        property(std::string_view name,
                 T value);

        property(std::string_view name);

        T
        value() const noexcept;

        void
        value(const T value);

        void
        write(std::ostream & stream) const;

        private:

        T _value = {};
    };


    template <>
    class property<std::string>: public ngmg::cypher::property_base
    {
        public:

        property(std::string_view name,
                 std::string_view value);

        property(std::string_view name);


        const std::string &
        value () const noexcept;

        void
        value(std::string_view value);

        void
        write(std::ostream & stream) const;

        private:

        std::string _value = {};
    };


    template <class T>
    struct is_property: public std::false_type {};

    template <class T>
    struct is_property<property<T>>: public std::true_type {};

    template <class Tuple>
    struct is_property_tuple;

    template <class ... Ts>
    struct is_property_tuple<std::tuple<Ts...>>: std::conjunction<is_property<std::remove_cvref_t<Ts>>...> {};


    template class property<int>;

    template <class T>
    property<T>::property(const std::string_view n,
                          T value):
        property_base(n),
        _value(value)
    {}

    template <class T>
    property<T>::property(const std::string_view n):
        property_base(n)
    {}

    template <class T>
    T
    property<T>::value() const noexcept
    {
        return this->_value;
    }

    template <class T>
    void
    property<T>::value(const T v)
    {
        this->_value = v;
    }

    namespace detail
    {
        template <class T>
        inline
        void
        write_value(std::ostream & stream,
                    T&& value)
        {
            if constexpr(std::is_same_v<std::remove_cvref_t<T>, std::string>)
                stream << '"' << std::forward<T>(value) << '"';
            else
                stream << std::forward<T>(value);
        }
    }

    template <class T>
    void
    property<T>::write(std::ostream & stream) const
    {
        stream << this->name() << ": ";
        ngmg::cypher::detail::write_value(stream, this->value());
    }

    template <class T, class U>
    inline
    std::strong_ordering
    operator <=> (const ngmg::cypher::property<T> & lhs, const ngmg::cypher::property<U> & rhs)
    {
        return static_cast<ngmg::cypher::property_base>(lhs) <=> static_cast<ngmg::cypher::property_base>(rhs);
    }

    template <class T, class U>
    inline
    bool
    operator == (const ngmg::cypher::property<T> & lhs, const ngmg::cypher::property<U> & rhs)
    {
        return static_cast<ngmg::cypher::property_base>(lhs) == static_cast<ngmg::cypher::property_base>(rhs);
    }


}

#endif
