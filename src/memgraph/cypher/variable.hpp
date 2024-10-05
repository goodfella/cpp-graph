#ifndef NGMG_CYPHER_VARIABLE_HPP
#define NGMG_CYPHER_VARIABLE_HPP

#include <string>
#include <string_view>

namespace ngmg::cypher
{
    namespace variable_tag
    {
        struct node;
        struct relationship;
    }

    template<class T>
    concept VariableTag =
        std::is_same_v<T, ngmg::cypher::variable_tag::node> ||
        std::is_same_v<T, ngmg::cypher::variable_tag::relationship>;

    namespace detail
    {
        class variable_base
        {
            public:

            explicit
            variable_base(const std::string_view name);

            std::string_view
            name() const noexcept;

            private:

            std::string _name;
        };
    }

    template <ngmg::cypher::VariableTag T>
    class variable: public ngmg::cypher::detail::variable_base
    {
        public:

        explicit
        variable(const std::string_view name);
    };

    template <ngmg::cypher::VariableTag T>
    variable<T>::variable(const std::string_view name):
        variable_base(name)
    {}

    using node_variable = variable<ngmg::cypher::variable_tag::node>;
    using relationship_variable = variable<ngmg::cypher::variable_tag::relationship>;

    template class variable<ngmg::cypher::variable_tag::node>;
    template class variable<ngmg::cypher::variable_tag::relationship>;
}

#endif
