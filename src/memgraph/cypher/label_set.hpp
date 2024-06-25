#ifndef NGMG_CYPHER_LABEL_SET_HPP
#define NGMG_CYPHER_LABEL_SET_HPP

#include "label.hpp"
#include <string_view>
#include <vector>

namespace ngmg::cypher
{
    class label_set
    {
        private:

        using container_type = std::vector<ngmg::cypher::label>;

        public:

        using iterator = container_type::iterator;
        using const_iterator = container_type::const_iterator;

        label_set() = default;

        bool
        insert(std::string_view label);

        const_iterator
        cbegin() const noexcept;

        const_iterator
        cend() const noexcept;

        bool
        empty() const noexcept;

        std::size_t
        size() const noexcept;

        private:

        container_type _container;
    };
}

#endif
