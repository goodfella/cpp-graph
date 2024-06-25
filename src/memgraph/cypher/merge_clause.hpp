#ifndef NGMG_MERGE_CLAUSE_HPP
#define NGMG_MERGE_CLAUSE_HPP

#include <concepts>
#include "relationship_expression.hpp"
#include <tuple>

namespace ngmg::cypher
{
    template <ngmg::cypher::RelationshipExpressionTuple Rel_Exps = std::tuple<>>
    class merge_clause
    {
        public:

        using relation_expression_tuple_type = Rel_Exps;

        explicit
        merge_clause(const Rel_Exps &);

        void
        write(std::ostream & stream) const;

        private:

        Rel_Exps _relationship_expressions;
    };

    template <class T>
    struct is_merge_clause: public std::false_type {};

    template <class T>
    struct is_merge_clause<merge_clause<T>>: public std::true_type {};

    template<ngmg::cypher::RelationshipExpressionTuple Rel_Exps>
    merge_clause<Rel_Exps>::merge_clause(const Rel_Exps & relationship_exps):
        _relationship_expressions(relationship_exps)
    {}

    template<ngmg::cypher::RelationshipExpressionTuple Rel_Exps>
    void
    merge_clause<Rel_Exps>::write(std::ostream & stream) const
    {
        if constexpr (std::tuple_size_v<Rel_Exps> > 0)
        {
            stream << "merge ";
        }

        if constexpr (std::tuple_size_v<Rel_Exps> > 0)
        {
            ngmg::cypher::write_relationship_expressions(stream, this->_relationship_expressions);
        }
    }
}

#endif
