#ifndef NGMG_NODE_EXPRESSION_HPP
#define NGMG_NODE_EXPRESSION_HPP

#include <functional>
#include "label.hpp"
#include <ostream>
#include "property_set.hpp"
#include <tuple>
#include <utility>
#include "variable.hpp"

namespace ngmg::cypher
{
    namespace detail
    {
        class node_expression_base
        {
            protected:

            node_expression_base(ngmg::cypher::variable const * variable,
                                 ngmg::cypher::label const * label);

            protected:

            ngmg::cypher::variable const * _var = nullptr;
            ngmg::cypher::label const * _label = nullptr;
        };
    }

    template <class Props>
    requires ngmg::cypher::is_property_tuple<Props>::value
    class node_expression: protected ngmg::cypher::detail::node_expression_base
    {
        public:

        using property_tuple_type = Props;

        node_expression(const Props & props);

        node_expression(std::reference_wrapper<const ngmg::cypher::variable> var,
                        const Props & props);

        node_expression(std::reference_wrapper<const ngmg::cypher::label> label,
                        const Props & props);

        node_expression(std::reference_wrapper<const ngmg::cypher::variable> var,
                        std::reference_wrapper<const ngmg::cypher::label> label,
                        const Props & props);

        void
        write(std::ostream & stream) const;

        private:

        Props _props;
    };

    template <class T>
    struct is_node_expression: public std::false_type {};

    template <class T>
    struct is_node_expression<node_expression<T>>: public std::true_type {};

    template <class Tuple>
    struct is_node_expression_tuple;

    template <class ... Ts>
    struct is_node_expression_tuple<std::tuple<Ts...>>:
        std::disjunction<std::is_same<std::tuple<>, std::tuple<Ts...>>,
                         std::conjunction<is_node_expression<std::remove_cvref_t<Ts>>...>> {};


    template <class Props>
    node_expression<Props>::node_expression(std::reference_wrapper<const ngmg::cypher::variable> var,
                                            const Props & props):
        node_expression_base(&var.get(), nullptr),
        _props {props}
    {}

    template <class Props>
    node_expression<Props>::node_expression(std::reference_wrapper<const ngmg::cypher::label> label,
                                            const Props & props):
        node_expression_base(nullptr, &label.get()),
        _props {props}
    {}

    template <class Props>
    node_expression<Props>::node_expression(const Props & props):
        node_expression_base(nullptr, nullptr),
        _props {props}
    {}

    template <class Props>
    node_expression<Props>::node_expression(std::reference_wrapper<const ngmg::cypher::variable> var,
                                            std::reference_wrapper<const ngmg::cypher::label> label,
                                            const Props &  props):
        node_expression_base(&var, &label),
        _props {props}
    {}

    template <class Props>
    void
    node_expression<Props>::write(std::ostream & stream) const
    {
        stream.put('(');

        if (this->_var)
        {
            stream << this->_var->name();
        }
        else
        {
            stream << "node";
        }

        if (this->_label)
        {
            stream << ':' << this->_label->name();
        }

        stream.put(' ');

        std::apply([&stream] (auto ... p) {ngmg::cypher::write_properties(stream, p...);}, this->_props);
        stream.put(')');
    }

    namespace detail
    {
        template <class Props, class ... Args>
        void
        write_node_expressions(std::ostream & stream, const ngmg::cypher::node_expression<Props> & ne, Args ... args)
        {
            ne.write(stream);

            if constexpr (sizeof...(args) > 0)
            {
                stream.write(", ", 2U);
                ngmg::cypher::detail::write_node_expressions(stream, args...);
            }
        }
    }

    template <class ... Args>
    void
    write_node_expressions(std::ostream & stream, Args ... args)
    {
        detail::write_node_expressions(stream, args...);
    }
}

#endif
