#ifndef NGMG_CREATE_CLAUSE_HPP
#define NGMG_CREATE_CLAUSE_HPP

#include <concepts>
#include "node_expression.hpp"
#include "relationship_expression.hpp"
#include <tuple>

namespace ngmg::cypher
{
    template <ngmg::cypher::NodeExpressionTuple Node_Exps = std::tuple<>,
              ngmg::cypher::RelationshipExpressionTuple Rel_Exps = std::tuple<>>
    class create_clause
    {
        public:

        using node_expression_tuple_type = Node_Exps;
        using relation_expression_tuple_type = Rel_Exps;

        explicit
        create_clause(const Node_Exps &);

        explicit
        create_clause(const Rel_Exps &);

        create_clause(const Node_Exps &, const Rel_Exps &);

        void
        write(std::ostream & stream) const;

        private:

        Node_Exps _node_expressions;
        Rel_Exps _relationship_expressions;
    };

    template <class T, class U = void>
    struct is_create_clause: public std::false_type {};

    template <class T, class U>
    struct is_create_clause<create_clause<T,U>>: public std::true_type {};

    template<ngmg::cypher::NodeExpressionTuple Node_Exps,
             ngmg::cypher::RelationshipExpressionTuple Rel_Exps>
    create_clause<Node_Exps, Rel_Exps>::create_clause(const Node_Exps & node_exps):
        _node_expressions(node_exps)
    {}

    template<ngmg::cypher::NodeExpressionTuple Node_Exps,
             ngmg::cypher::RelationshipExpressionTuple Rel_Exps>
    create_clause<Node_Exps, Rel_Exps>::create_clause(const Rel_Exps & relationship_exps):
        _relationship_expressions(relationship_exps)
    {}

    template<ngmg::cypher::NodeExpressionTuple Node_Exps,
             ngmg::cypher::RelationshipExpressionTuple Rel_Exps>
    create_clause<Node_Exps, Rel_Exps>::create_clause(const Node_Exps & node_exps,
                                                      const Rel_Exps & relationship_exps):
        _node_expressions(node_exps),
        _relationship_expressions(relationship_exps)
    {}

    template<ngmg::cypher::NodeExpressionTuple Node_Exps,
             ngmg::cypher::RelationshipExpressionTuple Rel_Exps>
    void
    create_clause<Node_Exps, Rel_Exps>::write(std::ostream & stream) const
    {
        if constexpr (std::tuple_size_v<Node_Exps> > 0 || std::tuple_size_v<Rel_Exps> > 0)
        {
            stream << "create ";
        }

        if constexpr (std::tuple_size_v<Node_Exps> > 0)
        {
            ngmg::cypher::write_node_expressions(stream, this->_node_expressions);
        }

        if constexpr (std::tuple_size_v<Rel_Exps> > 0)
        {
            ngmg::cypher::write_relationship_expressions(stream, this->_relationship_expressions);
        }
    }
}

#endif
