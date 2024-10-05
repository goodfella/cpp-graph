#ifndef NGMG_CYPHER_LABEL_HPP
#define NGMG_CYPHER_LABEL_HPP

#include <string>

namespace ngmg::cypher
{
    class label
    {
        public:

        label() = default;

        explicit
        label(std::string_view name);

        std::string_view
        name() const noexcept;

        void
        name(const std::string_view value);

        private:

        std::string _name;
    };

    inline
    std::strong_ordering
    operator <=> (const ngmg::cypher::label & lhs, const ngmg::cypher::label & rhs)
    {
        return lhs.name() <=> rhs.name();
    }

    inline
    bool
    operator == (const ngmg::cypher::label & lhs, const ngmg::cypher::label & rhs)
    {
        return lhs.name() == rhs.name();
    }

    inline
    std::strong_ordering
    operator <=> (const ngmg::cypher::label & lhs, std::string_view rhs)
    {
        return lhs.name() <=> rhs;
    }

    inline
    bool
    operator == (const ngmg::cypher::label & lhs, std::string_view rhs)
    {
        return lhs.name() == rhs;
    }
}

#endif
