#ifndef RAW_NODE_HPP
#define RAW_NODE_HPP

#include <clang-c/Index.h>
#include "memgraph/cypher/label.hpp"
#include "memgraph/cypher/property.hpp"
#include "memgraph/cypher/property_set.hpp"
#include "ngclang.hpp"
#include <set>
#include <string>
#include <tuple>

class raw_node
{
    public:

    raw_node();

    void
    fill(CXCursor cursor);

    void
    fill_match_props(CXCursor cursor);

    void
    fill_non_match_props(CXCursor cursor);

    ngmg::cypher::property<int> line_property;
    ngmg::cypher::property<int> column_property;
    ngmg::cypher::property<std::string> file_property;
    ngmg::cypher::property<int> kind_property;
    ngmg::cypher::property<bool> templated_property;
    ngmg::cypher::property<bool> visited_property;
    ngmg::cypher::property<std::string> type_spelling;
    ngmg::cypher::property<std::string> display_name;
    ngmg::cypher::property<int> instance_kind_property;

    auto
    match_property_tuple() const -> decltype(std::tie(line_property,
                                                      column_property,
                                                      file_property,
                                                      kind_property));

    auto
    property_tuple() const -> decltype(std::tie(line_property,
                                                column_property,
                                                file_property,
                                                kind_property,
                                                templated_property,
                                                visited_property,
                                                type_spelling,
                                                display_name,
                                                instance_kind_property));

    void
    clear_sets();

    ngmg::cypher::property_set property_set;
    std::set<ngmg::cypher::label> label_set;
};

#endif
