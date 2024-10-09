#ifndef NGMG_NODE_EXPRESSION_HPP
#define NGMG_NODE_EXPRESSION_HPP

#include <functional>
#include "label.hpp"
#include <ostream>
#include "property_set.hpp"
#include <set>
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

            node_expression_base(ngmg::cypher::node_variable const * variable,
                                 ngmg::cypher::label const * label,
                                 ngmg::cypher::property_set const * property_set,
                                 std::set<ngmg::cypher::label> const * label_set);

            protected:

            ngmg::cypher::node_variable const * _var = nullptr;
            ngmg::cypher::label const * _label = nullptr;
            ngmg::cypher::property_set const * _property_set = nullptr;
            std::set<ngmg::cypher::label> const * _label_set = nullptr;
        };
    }

    template <ngmg::cypher::PropertyTuple Props>
    class node_expression: protected ngmg::cypher::detail::node_expression_base
    {
        public:

        using property_tuple_type = Props;

        node_expression(const Props & props);

        node_expression(std::reference_wrapper<const ngmg::cypher::node_variable> var,
                        const Props & props);

        node_expression(std::reference_wrapper<const ngmg::cypher::label> label,
                        const Props & props);

        node_expression(std::reference_wrapper<ngmg::cypher::label> label,
                        const Props & props,
                        std::reference_wrapper<const ngmg::cypher::property_set> property_set);

        node_expression(std::reference_wrapper<const std::set<ngmg::cypher::label>> label_set,
                        const Props & props,
                        std::reference_wrapper<const ngmg::cypher::property_set> property_set);

        node_expression(std::reference_wrapper<const std::set<ngmg::cypher::label>> label_set,
                        const Props & props);

        node_expression(std::reference_wrapper<const ngmg::cypher::node_variable> var,
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

    template <class T>
    concept NodeExpression = ngmg::cypher::is_node_expression<T>::value;

    template <class T>
    concept NodeExpressionTuple = ngmg::cypher::is_node_expression_tuple<T>::value;

    template <class T>
    concept NodeRepresentation = NodeExpression<T> || std::is_same_v<T, ngmg::cypher::node_variable>;

    template <ngmg::cypher::PropertyTuple Props>
    node_expression<Props>::node_expression(std::reference_wrapper<const ngmg::cypher::node_variable> var,
                                            const Props & props):
        node_expression_base(&var.get(), nullptr, nullptr, nullptr),
        _props {props}
    {}

    template <ngmg::cypher::PropertyTuple Props>
    node_expression<Props>::node_expression(std::reference_wrapper<const ngmg::cypher::label> label,
                                            const Props & props):
        node_expression_base(nullptr, &label.get(), nullptr, nullptr),
        _props {props}
    {}

    template <ngmg::cypher::PropertyTuple Props>
    node_expression<Props>::node_expression(std::reference_wrapper<ngmg::cypher::label> label,
                                            const Props & props,
                                            std::reference_wrapper<const ngmg::cypher::property_set> property_set):
        node_expression_base(nullptr, &label.get(), &property_set.get(), nullptr),
        _props {props}
    {}

    template <ngmg::cypher::PropertyTuple Props>
    node_expression<Props>::node_expression(std::reference_wrapper<const std::set<ngmg::cypher::label>> label_set,
                                            const Props & props,
                                            std::reference_wrapper<const ngmg::cypher::property_set> property_set):
        node_expression_base(nullptr, nullptr, &property_set.get(), &label_set.get()),
        _props {props}
    {}

    template <ngmg::cypher::PropertyTuple Props>
    node_expression<Props>::node_expression(std::reference_wrapper<const std::set<ngmg::cypher::label>> label_set,
                                            const Props & props):
        node_expression_base(nullptr, nullptr, nullptr, &label_set.get()),
        _props {props}
    {}

    template <ngmg::cypher::PropertyTuple Props>
    node_expression<Props>::node_expression(const Props & props):
        node_expression_base(nullptr, nullptr, nullptr, nullptr),
        _props {props}
    {}

    template <ngmg::cypher::PropertyTuple Props>
    node_expression<Props>::node_expression(std::reference_wrapper<const ngmg::cypher::node_variable> var,
                                            std::reference_wrapper<const ngmg::cypher::label> label,
                                            const Props &  props):
        node_expression_base(&var.get(), &label.get(), nullptr, nullptr),
        _props {props}
    {}

    template <ngmg::cypher::PropertyTuple Props>
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

        if (this->_label_set)
        {
            for (auto & l: *this->_label_set)
            {
                stream << ':' << l.name();
            }
        }

        stream.put(' ');

        constexpr bool tuple_props_exist = std::tuple_size_v<Props> > 0;
        const bool property_set_props_exist = this->_property_set;

        if (tuple_props_exist && property_set_props_exist)
        {
            ngmg::cypher::write_property_collections(stream, this->_props, *this->_property_set);
        }
        else if (property_set_props_exist)
        {
            ngmg::cypher::write_property_set(stream, *this->_property_set);
        }
        else if constexpr (tuple_props_exist)
        {
            ngmg::cypher::write_properties(stream, this->_props);
        }

        stream.put(')');
    }

    namespace detail
    {
        template <class Props, class ... Args>
        void
        write_node_expressions(std::ostream & stream, const ngmg::cypher::node_expression<Props> & ne, Args && ... args)
        {
            ne.write(stream);

            if constexpr (sizeof...(args) > 0)
            {
                stream.write(", ", 2U);
                ngmg::cypher::detail::write_node_expressions(stream, std::forward<Args>(args)...);
            }
        }

        template <class ... Args>
        requires ngmg::cypher::is_node_expression_tuple<std::tuple<Args...>>::value
        void
        write_node_expressions(std::ostream & stream, const std::tuple<Args...> & node_expressions)
        {
            std::apply([&stream] (auto const & ... ne) {ngmg::cypher::detail::write_node_expressions(stream,ne...);}, node_expressions);
        }
    }

    template <class ... Args>
    void
    write_node_expressions(std::ostream & stream, Args && ... args)
    {
        ngmg::cypher::detail::write_node_expressions(stream, std::forward<Args>(args)...);
    }
}

#endif
