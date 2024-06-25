#ifndef NGMG_RELATIONSHIP_EXPRESSION_HPP
#define NGMG_RELATIONSHIP_EXPRESSION_HPP

#include <functional>
#include "label.hpp"
#include "property.hpp"
#include "property_set.hpp"
#include <ostream>
#include <string_view>
#include <type_traits>
#include "variable.hpp"

namespace ngmg::cypher
{
    enum class relationship_type
    {
        directed,
        undirected
    };

    namespace detail
    {
        class relationship_expression_base
        {
            protected:

            relationship_expression_base(const ngmg::cypher::variable & src,
                                         const ngmg::cypher::label & label,
                                         const ngmg::cypher::variable & dst,
                                         const ngmg::cypher::relationship_type type);

            protected:

            std::string_view _src;
            std::string_view _dst;
            std::string_view _label;
            ngmg::cypher::relationship_type _type;
        };
    }

    template <class Props = std::tuple<>>
    requires ngmg::cypher::is_property_tuple<Props>::value
    class relationship_expression: public detail::relationship_expression_base
    {
        public:

        using property_tuple_type = Props;

        relationship_expression(std::reference_wrapper<const ngmg::cypher::variable> src,
                                std::reference_wrapper<const ngmg::cypher::label> label,
                                std::reference_wrapper<const ngmg::cypher::variable> dst,
                                const relationship_type type);

        relationship_expression(std::reference_wrapper<const ngmg::cypher::variable> src,
                                std::reference_wrapper<const ngmg::cypher::label> label,
                                std::reference_wrapper<const ngmg::cypher::variable> dst,
                                const relationship_type type,
                                const Props & props);

        void
        write(std::ostream & stream) const;

        private:

        Props _props;
    };

    template <class T>
    struct is_relationship_expression: public std::false_type {};

    template <class T>
    struct is_relationship_expression<ngmg::cypher::relationship_expression<T>>: public std::true_type {};

    template <class Tuple>
    struct is_relationship_expression_tuple;

    template <class ... Ts>
    struct is_relationship_expression_tuple<std::tuple<Ts...>>:
        std::disjunction<std::is_same<std::tuple<>, std::tuple<Ts...>>,
                         std::conjunction<is_relationship_expression<std::remove_cvref_t<Ts>>...>> {};


    template <class Props>
    requires std::is_same<Props, std::tuple<>>::value
    relationship_expression<Props>::relationship_expression(std::reference_wrapper<const ngmg::cypher::variable> src,
                                                            std::reference_wrapper<const ngmg::cypher::label> label,
                                                            std::reference_wrapper<const ngmg::cypher::variable> dst,
                                                            const relationship_type type):
        relationship_expression_base(src, label, dst, type)
    {}

    template <class Props>
    relationship_expression<Props>::relationship_expression(std::reference_wrapper<const ngmg::cypher::variable> src,
                                                            std::reference_wrapper<const ngmg::cypher::label> label,
                                                            std::reference_wrapper<const ngmg::cypher::variable> dst,
                                                            const relationship_type type,
                                                            const Props & props):
        relationship_expression_base(src, label, dst, type),
        _props {props}
    {}

    template <class Props>
    void
    relationship_expression<Props>::write(std::ostream & stream) const
    {
        stream << '(' << this->_src << ")-";
        stream << "[:";
        stream << this->_label;
        if constexpr (std::tuple_size_v<decltype(this->_props)> > 0)
        {
            stream.put(' ');
            std::apply([&stream] (auto ... p) {ngmg::cypher::write_properties(stream, p...);}, this->_props);
        }

        stream.put(']');

        if (this->_type == ngmg::cypher::relationship_type::directed)
        {
            stream << "->";
        }
        else
        {
            stream.put('-');
        }

        stream << '(' << this->_dst << ')';
    }
}

#endif
