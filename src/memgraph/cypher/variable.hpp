#ifndef NGMG_CYPHER_VARIABLE_HPP
#define NGMG_CYPHER_VARIABLE_HPP

#include <string>
#include <string_view>

namespace ngmg::cypher
{
    class variable
    {
        public:

        explicit
        variable(const std::string_view name);

        std::string_view
        name() const noexcept;

        private:

        std::string _name;
    };
}

#endif
