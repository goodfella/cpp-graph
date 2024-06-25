#ifndef NGMG_RELATIONSHIP_EXPRESSION_HPP
#define NGMG_RELATIONSHIP_EXPRESSION_HPP

#include <functional>
#include "label.hpp"
#include "node_expression.hpp"
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

            relationship_expression_base(ngmg::cypher::label const * label,
                                         ngmg::cypher::relationship_variable const * variable,
                                         const ngmg::cypher::relationship_type type);

            void
            write_start(std::ostream & stream) const;

            void
            write_end(std::ostream & stream) const;

            ngmg::cypher::label const * _label = nullptr;
            ngmg::cypher::relationship_variable const * _variable = nullptr;
            ngmg::cypher::relationship_type _type;
        };

        template <class T>
        class relationship_expression_source_base;

        template <class T>
        class relationship_expression_source_base<ngmg::cypher::node_expression<T>>
        {
            protected:

            relationship_expression_source_base(std::reference_wrapper<const ngmg::cypher::node_expression<T>> src);

            void
            write_source(std::ostream & stream) const;

            ngmg::cypher::node_expression<T> const * _src;
        };

        template <class T>
        relationship_expression_source_base<ngmg::cypher::node_expression<T>>::relationship_expression_source_base(std::reference_wrapper<const ngmg::cypher::node_expression<T>> src):
            _src(&src.get())
        {}

        template <class T>
        void
        relationship_expression_source_base<ngmg::cypher::node_expression<T>>::write_source(std::ostream & stream) const
        {
            this->_src->write(stream);
        }

        template <>
        class relationship_expression_source_base<ngmg::cypher::node_variable>
        {
            protected:

            relationship_expression_source_base(const ngmg::cypher::node_variable & src);

            void
            write_source(std::ostream & stream) const;

            std::string_view _src;
        };

        template <class T>
        class relationship_expression_destination_base;

        template <class T>
        class relationship_expression_destination_base<ngmg::cypher::node_expression<T>>
        {
            protected:

            relationship_expression_destination_base(std::reference_wrapper<const ngmg::cypher::node_expression<T>> dst);

            void
            write_destination(std::ostream & stream) const;

            ngmg::cypher::node_expression<T> const * _dst;
        };

        template <class T>
        relationship_expression_destination_base<ngmg::cypher::node_expression<T>>::relationship_expression_destination_base(std::reference_wrapper<const ngmg::cypher::node_expression<T>> dst):
            _dst(&dst.get())
        {}

        template <class T>
        void
        relationship_expression_destination_base<ngmg::cypher::node_expression<T>>::write_destination(std::ostream & stream) const
        {
            this->_dst->write(stream);
        }

        template <>
        class relationship_expression_destination_base<ngmg::cypher::node_variable>
        {
            protected:

            relationship_expression_destination_base(const ngmg::cypher::node_variable & dst);

            void
            write_destination(std::ostream & stream) const;

            std::string_view _dst;
        };
    }

    template <ngmg::cypher::NodeRepresentation Src_Node,
              ngmg::cypher::NodeRepresentation Dst_Node,
              ngmg::cypher::PropertyTuple Props = std::tuple<>>

    class relationship_expression:
        private detail::relationship_expression_base,
        private detail::relationship_expression_source_base<Src_Node>,
        private detail::relationship_expression_destination_base<Dst_Node>
    {
        public:

        using property_tuple_type = Props;
        using src_node_type = Src_Node;
        using dst_node_type = Dst_Node;

        relationship_expression(std::reference_wrapper<const Src_Node> src,
                                std::reference_wrapper<const ngmg::cypher::label> label,
                                std::reference_wrapper<const Dst_Node> dst,
                                const relationship_type type);

        relationship_expression(std::reference_wrapper<const Src_Node> src,
                                std::reference_wrapper<const ngmg::cypher::label> label,
                                std::reference_wrapper<const Dst_Node> dst,
                                const relationship_type type,
                                const Props & props);

        relationship_expression(std::reference_wrapper<const ngmg::cypher::relationship_variable> var,
                                std::reference_wrapper<const Src_Node> src,
                                std::reference_wrapper<const ngmg::cypher::label> label,
                                std::reference_wrapper<const Dst_Node> dst,
                                const relationship_type type,
                                const Props & props);

        void
        write(std::ostream & stream) const;

        private:

        Props _props;
    };

    template <class T, class U = void, class V = void>
    struct is_relationship_expression: public std::false_type {};

    template <class T, class U, class V>
    struct is_relationship_expression<ngmg::cypher::relationship_expression<T, U, V>>: public std::true_type {};

    template <class Tuple>
    struct is_relationship_expression_tuple;

    template <class ... Ts>
    struct is_relationship_expression_tuple<std::tuple<Ts...>>:
        std::disjunction<std::is_same<std::tuple<>, std::tuple<Ts...>>,
                         std::conjunction<is_relationship_expression<std::remove_cvref_t<Ts>>...>> {};

    template <class T>
    concept RelationshipExpressionTuple = ngmg::cypher::is_relationship_expression_tuple<T>::value;

    template <ngmg::cypher::NodeRepresentation Src_Node,
	      ngmg::cypher::NodeRepresentation Dst_Node,
	      ngmg::cypher::PropertyTuple Props>
    relationship_expression<Src_Node, Dst_Node, Props>::relationship_expression(std::reference_wrapper<const Src_Node> src,
                                                                                std::reference_wrapper<const ngmg::cypher::label> label,
                                                                                std::reference_wrapper<const Dst_Node> dst,
                                                                                const relationship_type type):
         detail::relationship_expression_base(&label.get(), nullptr, type),
         detail::relationship_expression_source_base<Src_Node>::relationship_expression_source_base(src),
         detail::relationship_expression_destination_base<Dst_Node>::relationship_expression_destination_base(dst)
    {}

    template <ngmg::cypher::NodeRepresentation Src_Node,
	      ngmg::cypher::NodeRepresentation Dst_Node,
	      ngmg::cypher::PropertyTuple Props>
    relationship_expression<Src_Node, Dst_Node, Props>::relationship_expression(std::reference_wrapper<const Src_Node> src,
                                                                                std::reference_wrapper<const ngmg::cypher::label> label,
                                                                                std::reference_wrapper<const Dst_Node> dst,
                                                                                const relationship_type type,
                                                                                const Props & props):
        detail::relationship_expression_base(&label.get(), nullptr, type),
        detail::relationship_expression_source_base<Src_Node>::relationship_expression_source_base(src),
        detail::relationship_expression_destination_base<Dst_Node>::relationship_expression_destination_base(dst),
        _props {props}
    {}

    template <ngmg::cypher::NodeRepresentation Src_Node,
	      ngmg::cypher::NodeRepresentation Dst_Node,
	      ngmg::cypher::PropertyTuple Props>
    relationship_expression<Src_Node, Dst_Node, Props>::relationship_expression(std::reference_wrapper<const ngmg::cypher::relationship_variable> variable,
                                                                                std::reference_wrapper<const Src_Node> src,
                                                                                std::reference_wrapper<const ngmg::cypher::label> label,
                                                                                std::reference_wrapper<const Dst_Node> dst,
                                                                                const relationship_type type,
                                                                                const Props & props):
        detail::relationship_expression_base(&label.get(), &variable.get(), type),
        detail::relationship_expression_source_base<Src_Node>::relationship_expression_source_base(src),
        detail::relationship_expression_destination_base<Dst_Node>::relationship_expression_destination_base(dst),
        _props {props}
    {}

    template <ngmg::cypher::NodeRepresentation Src_Node,
	      ngmg::cypher::NodeRepresentation Dst_Node,
	      ngmg::cypher::PropertyTuple Props>
    void
    relationship_expression<Src_Node, Dst_Node, Props>::write(std::ostream & stream) const
    {
        this->write_source(stream);
        this->write_start(stream);

        if constexpr (std::tuple_size_v<decltype(this->_props)> > 0)
        {
            ngmg::cypher::write_properties(stream, this->_props);
        }

        this->write_end(stream);
        this->write_destination(stream);
    }

    namespace detail
    {
        template <class Src_Node, class Dst_Node, class Props, class ... Args>
        void
        write_relationship_expressions(std::ostream & stream, const ngmg::cypher::relationship_expression<Src_Node, Dst_Node, Props> & re, Args && ... args)
        {
            re.write(stream);

            if constexpr (sizeof...(args) > 0)
            {
                stream.write(", ", 2U);
                ngmg::cypher::detail::write_relationship_expressions(stream, std::forward<Args>(args)...);
            }
        }

        template <class ... Args>
        requires ngmg::cypher::is_relationship_expression_tuple<std::tuple<Args...>>::value
        void
        write_relationship_expressions(std::ostream & stream, const std::tuple<Args...> & relationship_expressions)
        {
            std::apply([&stream] (auto const & ... ne) {ngmg::cypher::detail::write_relationship_expressions(stream,ne...);}, relationship_expressions);
        }
    }

    template <class ... Args>
    void
    write_relationship_expressions(std::ostream & stream, Args && ... args)
    {
        ngmg::cypher::detail::write_relationship_expressions(stream, std::forward<Args>(args)...);
    }
}

#endif
