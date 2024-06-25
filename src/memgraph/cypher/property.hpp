#ifndef NGMG_CYPHER_PROPERTY_HPP
#define NGMG_CYPHER_PROPERTY_HPP

#include <compare>
#include <iomanip>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
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

    using memgraph_data_types =
        std::tuple<std::string,
                   bool,
                   int,
                   float>;

    template <class T, class Tuple>
    struct tuple_contains;

    template <class T>
    struct tuple_contains<T, std::tuple<>> : std::false_type {};

    template <class T, class U, class ... Ts>
    struct tuple_contains<T, std::tuple<U, Ts...>> : tuple_contains<T, std::tuple<Ts...>> {};

    template <class T, class ... Ts>
    struct tuple_contains<T, std::tuple<T, Ts...>>: std::true_type {};

    template <class T>
    concept MemgraphDataType =
        tuple_contains<T, ngmg::cypher::memgraph_data_types>::value;

    template <ngmg::cypher::MemgraphDataType T>
    class property: public ngmg::cypher::property_base
    {
        public:

        using value_type = T;

        property(std::string_view name,
                 T value);

        property(std::string_view name);

        T
        value() const noexcept;

        void
        value(const T value);

        property &
        operator = (const T value);

        void
        write(std::ostream & stream) const;

        void
        write(std::ostream & stream, const std::string_view prefix) const;

        private:

        T _value = {};
    };


    template <>
    class property<std::string>: public ngmg::cypher::property_base
    {
        public:

        using value_type = std::string;

        property(std::string_view name,
                 std::string_view value);

        property(std::string_view name);


        const std::string &
        value () const noexcept;

        void
        value(std::string_view value);

        property<std::string> &
        operator = (std::string_view value);

        void
        write(std::ostream & stream) const;

        private:

        std::string _value = {};
    };


    template <class T>
    struct is_property: public std::false_type {};

    template <class T>
    struct is_property<property<T>>: public std::true_type {};

    template <class T>
    concept Property = ngmg::cypher::is_property<T>::value;

    template <class Tuple>
    struct is_property_tuple;

    template <class ... Ts>
    struct is_property_tuple<std::tuple<Ts...>>: std::conjunction<is_property<std::remove_cvref_t<Ts>>...> {};

    template <class T>
    concept PropertyTuple = ngmg::cypher::is_property_tuple<T>::value;


    template <class Tuple>
    struct is_property_name_tuple;

    template <class ... Ts>
    struct is_property_name_tuple<std::tuple<Ts...>>: std::conjunction<std::is_same<std::string_view, std::remove_cvref_t<Ts>>...> {};

    template <class T>
    concept PropertyNameTuple = ngmg::cypher::is_property_name_tuple<T>::value;


    template class property<int>;

    template <class T>
    struct to_variant;

    template <class ... Ts>
    struct to_variant<std::tuple<Ts...>>
    {
        using type = std::variant<ngmg::cypher::property<Ts>...>;
    };

    using property_variant = to_variant<ngmg::cypher::memgraph_data_types>::type;

    void
    write_property(std::ostream & stream, const ngmg::cypher::property_variant & property);

    template <MemgraphDataType T>
    property<T>::property(const std::string_view n,
                          T value):
        property_base(n),
        _value(value)
    {}

    template <MemgraphDataType T>
    property<T>::property(const std::string_view n):
        property_base(n)
    {}

    template <MemgraphDataType T>
    T
    property<T>::value() const noexcept
    {
        return this->_value;
    }

    template <MemgraphDataType T>
    void
    property<T>::value(const T v)
    {
        this->_value = v;
    }

    template <MemgraphDataType T>
    property<T> &
    property<T>::operator = (const T v)
    {
        this->value(v);
        return *this;
    }

    namespace detail
    {
        template <class T>
        inline
        void
        write_value(std::ostream & stream,
                    T&& value)
        {
            if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string>)
            {
                stream.put('"');
                for (const auto & c: value)
                {
                    if (c == '"' || c == '\'')
                    {
                        stream.put('\\');
                        stream.put(c);
                    }
                    else
                    {
                        stream.put(c);
                    }
                }
                stream.put('"');
            }
            else if constexpr (std::is_same_v<std::remove_cvref_t<T>, bool>)
            {
                stream << std::boolalpha << std::forward<T>(value);
            }
            else
            {
                stream << std::forward<T>(value);
            }
        }
    }

    template <ngmg::cypher::MemgraphDataType T>
    void
    property<T>::write(std::ostream & stream) const
    {
        stream << this->name() << ": ";
        ngmg::cypher::detail::write_value(stream, this->value());
    }

    template <ngmg::cypher::MemgraphDataType T>
    void
    property<T>::write(std::ostream & stream, const std::string_view prefix) const
    {
        stream << prefix << '.' << this->name() << " = ";
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

    namespace detail
    {
        template <class T, class ... Args>
        void
        write_properties(std::ostream & stream, const ngmg::cypher::property<T> & property, Args && ... args)
        {
            property.write(stream);

            if constexpr (sizeof...(args) > 0)
            {
                stream.write(", ", 2U);
                ngmg::cypher::detail::write_properties(stream, std::forward<Args>(args)...);
            }
        }

        template <class T, class ... Args>
        void
        write_properties(std::ostream & stream,
                         const std::string_view prefix,
                         const ngmg::cypher::property<T> & property, Args && ... args)
        {
            property.write(stream, prefix);

            if constexpr (sizeof...(args) > 0)
            {
                stream.write(", ", 2U);
                ngmg::cypher::detail::write_properties(stream, prefix, std::forward<Args>(args)...);
            }
        }

        template <class ... Args>
        void
        write_property_names(std::ostream & stream,
                             const std::string_view & prefix,
                             const std::string_view & name,
                             Args && ... args)
        {
            stream << prefix << '.' << name;

            if constexpr (sizeof...(args) > 0)
            {
                stream.write(", ", 2U);
                ngmg::cypher::detail::write_property_names(stream, prefix, std::forward<Args>(args)...);
            }
        }

        template <class ... Args>
        requires ngmg::cypher::is_property_tuple<std::tuple<Args...>>::value
        void
        write_properties(std::ostream & stream, const std::tuple<Args...> & props)
        {
            std::apply([&stream] (auto const & ... p) {ngmg::cypher::detail::write_properties(stream, p...);}, props);
        }

        template <class ... Args>
        requires ngmg::cypher::is_property_tuple<std::tuple<Args...>>::value
        void
        write_properties(std::ostream & stream,
                         const std::string_view prefix,
                         const std::tuple<Args...> & props)
        {
            std::apply([&stream, prefix] (auto const & ... p) {ngmg::cypher::detail::write_properties(stream, prefix, p...);}, props);
        }

        template <class ... Args>
        requires ngmg::cypher::is_property_name_tuple<std::tuple<Args...>>::value
        void
        write_property_names(std::ostream & stream,
                             const std::string_view & prefix,
                             const std::tuple<Args...> & props)
        {
            std::apply([&stream, prefix] (auto const & ... p) {ngmg::cypher::detail::write_property_names(stream, prefix, p...);}, props);
        }
    }

    template <class ... Args>
    void
    write_properties(std::ostream & stream, Args && ... args)
    {
        stream.put('{');
        ngmg::cypher::detail::write_properties(stream, std::forward<Args>(args)...);
        stream.put('}');
    }

    template <class ... Args>
    void
    write_properties(std::ostream & stream, const std::string_view prefix, Args && ... args)
    {
        ngmg::cypher::detail::write_properties(stream, prefix, std::forward<Args>(args)...);
    }

    template <class ... Args>
    void
    write_property_names(std::ostream & stream, const std::string_view & prefix, Args && ... args)
    {
        ngmg::cypher::detail::write_property_names(stream, prefix, std::forward<Args>(args)...);
    }

    template <class ... Props>
    auto
    property_names(const std::tuple<Props...> & props)
    {
        return std::apply([] (auto const & ... p) {return std::make_tuple(p.name()...);}, props);
    }
}

#endif
