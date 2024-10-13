#include "ngclang.hpp"
#include "node_property_names.hpp"
#include "raw_node.hpp"
#include <sstream>

void
fill_kind_label_set(const CXCursorKind kind, std::set<ngmg::cypher::label> & label_set)
{
    std::string spelling;

    // Handle any spellings that are not valid labels
    switch(kind)
    {
        case CXCursor_CXXBaseSpecifier:
        {
            spelling = "CXXBaseClassSpecifier";
            break;
        }
        case CXCursor_UnexposedAttr:
        {
            spelling = "UnexposedAttr";
            break;
        }
        case CXCursor_CXXOverrideAttr:
        {
            spelling = "CXXOverrideAttr";
            break;
        }
        case CXCursor_CXXFinalAttr:
        {
            spelling = "CXXFinalAttr";
            break;
        }
        case CXCursor_AlignedAttr:
        {
            spelling = "AlignedAttr";
            break;
        }
        default:
        {
            const ngclang::string_t clang_str = clang_getCursorKindSpelling(kind);
            spelling = ngclang::to_string(clang_str.get());
            break;
        }
    }

    label_set.emplace(spelling);
}

raw_node::raw_node():
    line_property {line_prop_name},
    column_property {column_prop_name},
    file_property {file_prop_name},
    kind_property {kind_prop_name},
    templated_property {templated_prop_name},
    visited_property {visited_prop_name},
    type_spelling {type_spelling_prop_name},
    display_name {display_name_prop_name},
    instance_kind_property {instance_kind_prop_name}
{}

void
raw_node::fill_match_props(CXCursor cursor)
{
    const CXCursorKind kind = clang_getCursorKind(cursor);
    const ngclang::cursor_location loc = ngclang::cursor_location(cursor);

    fill_kind_label_set(kind, this->label_set);
    this->kind_property.value(kind);
    this->column_property.value(loc.column());
    this->file_property.value(loc.file());
    this->line_property.value(loc.line());
}

void
raw_node::fill_non_match_props(CXCursor cursor)
{
    const CXCursorKind kind = clang_getCursorKind(cursor);
    this->templated_property.value((kind == CXCursor_FunctionTemplate ||
                                    kind == CXCursor_ClassTemplate ||
                                    kind == CXCursor_ClassTemplatePartialSpecialization));

    if (this->templated_property.value())
    {
        const CXCursorKind instance_kind = clang_getTemplateCursorKind(cursor);
        this->instance_kind_property = instance_kind;
    }
    else
    {
        this->instance_kind_property = -1;
    }

    const ngclang::universal_symbol_reference usr {cursor};

    if (!usr.string().empty())
    {
        this->property_set.emplace(usr_prop_name, usr.string());
    }

    const CXType cursor_type = clang_getCursorType(cursor);

    this->type_spelling.value(ngclang::to_string(clang_getTypeSpelling(cursor_type)));
    this->display_name.value(ngclang::to_string(cursor, &clang_getCursorDisplayName));
}

void
raw_node::fill(CXCursor cursor)
{
    this->fill_match_props(cursor);
    this->fill_non_match_props(cursor);
}

auto
raw_node::property_tuple() const -> decltype(std::tie(this->line_property,
                                                      this->column_property,
                                                      this->file_property,
                                                      this->kind_property,
                                                      this->templated_property,
                                                      this->visited_property,
                                                      this->type_spelling,
                                                      this->display_name,
                                                      this->instance_kind_property))
{
    return std::tie(this->line_property,
                    this->column_property,
                    this->file_property,
                    this->kind_property,
                    this->templated_property,
                    this->visited_property,
                    this->type_spelling,
                    this->display_name,
                    this->instance_kind_property);
}

auto
raw_node::match_property_tuple() const -> decltype(std::tie(line_property,
                                                            column_property,
                                                            file_property,
                                                            kind_property))
{
    return std::tie(this->line_property,
                    this->column_property,
                    this->file_property,
                    this->kind_property);
}

void
raw_node::clear_sets()
{
    this->property_set.clear();
    this->label_set.clear();
}
