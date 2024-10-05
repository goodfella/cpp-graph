#ifndef NGMG_CYPHER_RETURN_CLAUSE_HPP
#define NGMG_CYPHER_RETURN_CLAUSE_HPP

#include <functional>
#include "property.hpp"
#include <string_view>
#include <tuple>
#include "variable.hpp"

namespace ngmg::cypher
{
    namespace detail
    {
        class return_clause_base
        {
            public:

            return_clause_base(const std::string_view variable);

            protected:

            std::string_view _variable;
        };
    }

    template <ngmg::cypher::PropertyNameTuple Props = std::tuple<>>
    class return_clause: private detail::return_clause_base
    {
        public:

        explicit
        return_clause(std::reference_wrapper<const ngmg::cypher::node_variable> variable);

        explicit
        return_clause(std::reference_wrapper<const ngmg::cypher::relationship_variable> variable);

        return_clause(std::reference_wrapper<const ngmg::cypher::node_variable> variable,
                      const Props & props);

        void
        write(std::ostream & stream) const;

        private:

        Props _props;
    };

    template <class T>
    struct is_return_clause: public std::false_type {};

    template <class T>
    struct is_return_clause<return_clause<T>>: public std::true_type {};


    template <ngmg::cypher::PropertyNameTuple Props>
    return_clause<Props>::return_clause(std::reference_wrapper<const ngmg::cypher::node_variable> variable):
        return_clause_base(variable.get().name())
    {}

    template <ngmg::cypher::PropertyNameTuple Props>
    return_clause<Props>::return_clause(std::reference_wrapper<const ngmg::cypher::relationship_variable> variable):
        return_clause_base(variable.get().name())
    {}

    template <ngmg::cypher::PropertyNameTuple Props>
    return_clause<Props>::return_clause(std::reference_wrapper<const ngmg::cypher::node_variable> variable,
                                        const Props & props):
        return_clause_base(variable.get().name()),
        _props(props)
    {}

    template <ngmg::cypher::PropertyNameTuple Props>
    void
    return_clause<Props>::write(std::ostream & stream) const
    {
        stream << "RETURN ";
        if constexpr (std::tuple_size_v<Props> > 0)
        {
            std::apply([&stream, this] (auto ... p)
                {ngmg::cypher::write_property_names(stream, this->_variable, p...);}, this->_props);
        }
        else
        {
            stream << this->_variable;
        }
    }
}

#endif
