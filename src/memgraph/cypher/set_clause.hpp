#ifndef NGMG_CYPHER_SET_CLAUSE_HPP
#define NGMG_CYPHER_SET_CLAUSE_HPP

#include <functional>
#include "property.hpp"
#include <string_view>
#include <tuple>
#include "variable.hpp"

namespace ngmg::cypher
{
    namespace detail
    {
        class set_clause_base
        {
            public:

            set_clause_base(const std::string_view variable);

            protected:

            std::string_view _variable;
        };
    }

    template <ngmg::cypher::PropertyTuple Props>
    class set_clause: private detail::set_clause_base
    {
        public:

        set_clause(std::reference_wrapper<const ngmg::cypher::node_variable> variable,
                   const Props & props);

        void
        write(std::ostream & stream) const;

        private:

        Props _props;
    };

    template <class T>
    struct is_set_clause: public std::false_type {};

    template <class T>
    struct is_set_clause<set_clause<T>>: public std::true_type {};

    template <ngmg::cypher::PropertyTuple Props>
    set_clause<Props>::set_clause(std::reference_wrapper<const ngmg::cypher::node_variable> variable,
                                  const Props & props):
        set_clause_base(variable.get().name()),
        _props(props)
    {}

    template <ngmg::cypher::PropertyTuple Props>
    void
    set_clause<Props>::write(std::ostream & stream) const
    {
        stream << "SET ";
        std::apply([&stream, this] (auto ... p)
            {ngmg::cypher::write_properties(stream, this->_variable, p...);}, this->_props);
    }
}

#endif
