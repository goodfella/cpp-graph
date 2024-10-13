#ifndef NODE_PROPERTY_NAMES_HPP
#define NODE_PROPERTY_NAMES_HPP

#include <string_view>

constexpr std::string_view kind_prop_name = "kind";
constexpr std::string_view instance_kind_prop_name = "instance_kind";
constexpr std::string_view line_prop_name = "line";
constexpr std::string_view column_prop_name = "column";
constexpr std::string_view file_prop_name = "file";
constexpr std::string_view usr_prop_name = "universal_symbol_reference";
constexpr std::string_view unvisited_prop_name = "unvisited";
constexpr std::string_view templated_prop_name = "templated";
constexpr std::string_view visited_prop_name = "visited";
constexpr std::string_view type_spelling_prop_name = "type_spelling";
constexpr std::string_view display_name_prop_name = "display_name";
constexpr std::string_view fq_name_prop_name = "fq_name";
constexpr std::string_view name_prop_name = "name";
constexpr std::string_view unqualified_name_prop_name = "unqualified_name";
constexpr std::string_view is_template_prop_name = "is_template";
constexpr std::string_view has_reference_prop_name = "has_reference";
constexpr std::string_view function_def_present_prop_name = "function_def_present";

#endif
