#include <algorithm>
#include <array>
#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>
#include "class_decl_node.hpp"
#include "class_node.hpp"
#include <cstring>
#include "edge_labels.hpp"
#include <exception>
#include <filesystem>
#include <functional>
#include "function_node.hpp"
#include "function_decl_def_node.hpp"
#include <getopt.h>
#include "help.hpp"
#include <iostream>
#include <memory>
#include "memgraph/cypher.hpp"
#include "memgraph/cypher/property.hpp"
#include "memgraph/cypher/property_set.hpp"
#include <mgclient.hpp>
#include "namespace_node.hpp"
#include "namespace_decl_node.hpp"
#include "ngclang.hpp"
#include "node_property_names.hpp"
#include <optional>
#include "raw_node.hpp"
#include <sstream>
#include "statement_executor.hpp"
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

class memgraph_init
{
    public:

    memgraph_init();
    ~memgraph_init();
};

memgraph_init::memgraph_init()
{
    mg::Client::Init();
}

memgraph_init::~memgraph_init()
{
    mg::Client::Finalize();
}

std::ostream& operator<<(std::ostream& stream, const CXString& str)
{
    char const * const cstring = clang_getCString(str);
    if (cstring)
    {
        stream << cstring;
    }
    clang_disposeString(str);
    return stream;
}

class cursor_location
{
    public:

    explicit
    cursor_location(CXCursor c);

    cursor_location(const int line, std::string_view file, const int column);

    explicit
    cursor_location(const ngclang::cursor_location & l);

    const std::string &
    file () const noexcept;

    int
    line() const noexcept;

    int
    column() const noexcept;

    private:

    std::string _file;
    int _line = {};
    int _column = {};
};

cursor_location::cursor_location(CXCursor cursor)
{
    const CXSourceLocation location = clang_getCursorLocation(cursor);


    unsigned int line;
    unsigned int column;

    CXFile file;
    clang_getExpansionLocation(location,
                               &file,
                               &line,
                               &column,
                               nullptr);

    this->_line = line;
    this->_column = column;


    ngclang::string_t path = clang_File_tryGetRealPathName(file);
    this->_file = ngclang::to_string(path.get());
}

cursor_location::cursor_location(const ngclang::cursor_location & l):
    _file(l.file()),
    _line(l.line()),
    _column(l.column())
{}

cursor_location::cursor_location(const int line, std::string_view file, const int column):
    _file(file),
    _line(line),
    _column(column)
{}

const std::string &
cursor_location::file() const noexcept
{
    return this->_file;
}

int
cursor_location::line() const noexcept
{
    return this->_line;
}

int
cursor_location::column() const noexcept
{
    return this->_column;
}

void
print_cursor(CXCursor c, CXCursor parent, unsigned int level)
{
    CXCursor semantic_parent = clang_getCursorSemanticParent(c);
    const std::string semantic_parent_usr = ngclang::to_string(semantic_parent, &clang_getCursorUSR);

    CXCursor lexical_parent = clang_getCursorLexicalParent(c);
    const std::string lexical_parent_usr = ngclang::to_string(lexical_parent, &clang_getCursorUSR);

    const CXCursorKind cursor_kind = clang_getCursorKind(c);
    const CXType cursor_type = clang_getCursorType(c);
    const CXSourceLocation location = clang_getCursorLocation(c);
    CXCursor ref = clang_getCursorReferenced(c);
    const std::string usr = ngclang::to_string(c, &clang_getCursorUSR);
    const std::string parent_usr = ngclang::to_string(parent, &clang_getCursorUSR);
    const std::string name = ngclang::to_string(c, &clang_getCursorSpelling);
    const std::string display_name = ngclang::to_string(c, &clang_getCursorDisplayName);

    unsigned line = 0;
    unsigned column = 0;
    CXFile file;

    clang_getFileLocation(location,
                          &file,
                          &line,
                          &column,
                          nullptr);

    const std::string cursor_indent = std::string(level, '-') + '>';
    const std::string cursor_property_indent = std::string(level + 1, ' ') + '|';
    std::cout << cursor_indent
              << clang_getCursorKindSpelling(cursor_kind) << std::endl
              << cursor_property_indent << " type "
              << '(' << clang_getTypeSpelling(cursor_type) << ')' << std::endl
              << cursor_property_indent << " usr "
              << '(' << usr << ')' << std::endl
              << cursor_property_indent << " parent usr "
              << '(' << parent_usr << ')' << std::endl
              << cursor_property_indent << " lexical parent usr "
              << '(' << lexical_parent_usr << ')' << std::endl
              << cursor_property_indent << " semantic parent usr "
              << '(' << semantic_parent_usr << ')' << std::endl
              << cursor_property_indent << " name "
              << '(' << name << ')' << std::endl
              << cursor_property_indent << " display name "
              << '(' << display_name << ')' << std::endl
              << cursor_property_indent << " file "
              << '(' << clang_getFileName(file) << ')' << std::endl
              << cursor_property_indent << " line "
              << '(' << line << ')' << std::endl
              << cursor_property_indent << " column "
              << '(' << column << ')' << std::endl;

    const std::string ref_property_indent = "  " + cursor_property_indent;
    const auto overloaded_decl_ref_count = clang_getNumOverloadedDecls(c);
    if (!clang_Cursor_isNull(ref))
    {
        const std::string ref_usr = ngclang::to_string(ref, &clang_getCursorUSR);
        std::cout << cursor_property_indent << " ref " << std::endl;
        std::cout << ref_property_indent << " type "
                  << '(' << clang_getTypeSpelling(clang_getCursorType(ref)) << ')' << std::endl;
        std::cout << ref_property_indent << " usr "
                  << '(' << ref_usr << ')' << std::endl;
    }
    else
    {
        std::cout << cursor_property_indent << " ref ";
        std::cout << '(' << "null" << ')' << std::endl;
    }

    if (overloaded_decl_ref_count > 0)
    {
        for (unsigned int i = 0; i < overloaded_decl_ref_count; ++i)
        {
            CXCursor overloaded_ref = clang_getOverloadedDecl(c, i);
            if (!clang_Cursor_isNull(overloaded_ref))
            {
                const std::string ref_usr = ngclang::to_string(overloaded_ref, &clang_getCursorUSR);
                std::cout << cursor_property_indent << " overloaded decl ref " << std::endl;
                std::cout << ref_property_indent << " type "
                          << '(' << clang_getTypeSpelling(clang_getCursorType(ref)) << ')' << std::endl;
                std::cout << ref_property_indent << " usr "
                          << '(' << ref_usr << ')' << std::endl;
            }
        }
    }

}

CXChildVisitResult
printing_visitor(CXCursor c, CXCursor parent, CXClientData client_data)
{
    const unsigned int level = *(reinterpret_cast<unsigned int * const>(client_data));
    print_cursor(c, parent, level);
    unsigned int next_level = level + 1;
    clang_visitChildren(c, &printing_visitor, &next_level);
    return CXChildVisit_Continue;
}

class name_decl
{
    public:
    name_decl(const std::string & name,
              const cursor_location & location);

    name_decl(const std::string & name,
              const ngclang::cursor_location & location);

    name_decl(std::string_view name);

    std::string
    name() const;

    private:

    std::string _name;
    cursor_location _location;
};

name_decl::name_decl(const std::string & name,
                     const cursor_location & location):
    _name(name),
    _location(location)
{}

name_decl::name_decl(std::string_view name):
    _name(name),
    _location(cursor_location {0, "", 0})
{}

std::string
name_decl::name() const
{
    return this->_name;
}

class function_decl
{
    public:

    explicit
    function_decl(CXCursor cursor);

    std::string
    universal_symbol_reference() const;

    cursor_location
    location() const;

    bool
    is_member_function() const noexcept;

    private:

    cursor_location _location;
    std::string _universal_symbol_reference;
    bool _is_member_function = false;
};

function_decl::function_decl(CXCursor cursor):
    _location(cursor),
    _universal_symbol_reference(ngclang::to_string(cursor, &clang_getCursorUSR))
{
    const CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    if (cursor_kind == CXCursor_CXXMethod)
    {
        this->_is_member_function = true;
    }
}

std::string
function_decl::universal_symbol_reference() const
{
    return this->_universal_symbol_reference;
}

cursor_location
function_decl::location() const
{
    return this->_location;
}

bool
function_decl::is_member_function() const noexcept
{
    return this->_is_member_function;
}

template <class T>
class vector_sentry
{
    public:

    vector_sentry(std::reference_wrapper<std::vector<T>> stack);
    ~vector_sentry();

    void
    push(const T);

    std::vector<T> * const _stack;
    bool _armed = false;
};

template <class T>
vector_sentry<T>::vector_sentry(std::reference_wrapper<std::vector<T>> stack):
    _stack(&stack.get())
{}

template <class T>
vector_sentry<T>::~vector_sentry()
{
    if (this->_armed)
    {
        this->_stack->pop_back();
    }
}

template <class T>
void
vector_sentry<T>::push(const T t)
{
    this->_stack->push_back(t);
    this->_armed= true;
}

template <class T>
class optional_sentry
{
    public:
    optional_sentry(const std::reference_wrapper<std::optional<T>> o);
    ~optional_sentry();

    void
    set(T);

    private:

    bool _armed = false;
    std::optional<T> * const _optional;

};

template <class T>
optional_sentry<T>::optional_sentry(const std::reference_wrapper<std::optional<T>> o):
    _optional(&o.get())
{}

template <class T>
void
optional_sentry<T>::set(const T t)
{
        *this->_optional = t;
        this->_armed = true;
}

template <class T>
optional_sentry<T>::~optional_sentry()
{
    if (this->_armed)
    {
        this->_optional->reset();
    }
}

class ast_visitor_filter
{
    public:

    ast_visitor_filter() = default;

    bool
    is_src_dir(const std::filesystem::path & path) const;

    bool
    in_src_tree(const std::filesystem::path & path) const;

    bool
    is_src_file(const std::filesystem::path & path) const noexcept;

    bool
    parse_file(const std::filesystem::path & file) const;

    void
    push_back_src_dir(const std::filesystem::path & src_dir);

    void
    push_back_src_tree(const std::filesystem::path & src_tree);

    void
    push_back_src_file(const std::filesystem::path & src_file);

    private:
    std::vector<std::filesystem::path> _src_dirs;
    std::vector<std::filesystem::path> _src_trees;
    std::vector<std::filesystem::path> _src_files;
};

bool
ast_visitor_filter::is_src_dir(const std::filesystem::path & file) const
{
    std::filesystem::path src_dir = file;
    src_dir.remove_filename();

    auto is_src_dir = [&src_dir] (const std::filesystem::path & p) {return std::filesystem::equivalent(src_dir, p);};
    return (std::find_if(this->_src_dirs.cbegin(), this->_src_dirs.cend(), is_src_dir) != this->_src_dirs.cend());
}

bool
ast_visitor_filter::is_src_file(const std::filesystem::path & file) const noexcept
{
    auto is_src_file = [&file] (const std::filesystem::path & p) {return std::filesystem::equivalent(file, p);};
    return (std::find_if(this->_src_files.cbegin(), this->_src_files.cend(), is_src_file) != this->_src_files.cend());
}

bool
ast_visitor_filter::in_src_tree(const std::filesystem::path & path) const
{
    for (auto & t: this->_src_trees)
    {
        auto ret = std::mismatch(t.begin(), t.end(), path.begin(), path.end());

        if (ret.first == t.end())
        {
            return true;
        }

        if (std::distance(ret.first, t.end()) == 1 && ret.first->empty())
        {
            return true;
        }
    }

    return false;
}

void
ast_visitor_filter::push_back_src_dir(const std::filesystem::path & src_dir)
{
    this->_src_dirs.push_back(src_dir);
}

void
ast_visitor_filter::push_back_src_tree(const std::filesystem::path & src_tree)
{
    this->_src_trees.push_back(src_tree);
}

void
ast_visitor_filter::push_back_src_file(const std::filesystem::path & file)
{
    this->_src_files.push_back(file);
}

bool
ast_visitor_filter::parse_file(const std::filesystem::path & file) const
{
    std::optional<bool> match_found;

    if (!this->_src_files.empty())
    {
        match_found = this->is_src_file(file);
    }

    if (match_found && *match_found)
    {
        return true;
    }

    if (!this->_src_dirs.empty())
    {
        match_found = this->is_src_dir(file);
    }

    if (match_found && *match_found)
    {
        return true;
    }

    if (!this->_src_trees.empty())
    {
        match_found = this->in_src_tree(file);
    }

    if (match_found)
    {
        return *match_found;
    }

    // no filter was given, so all files should be parsed
    return true;
}

class ast_visitor_policy
{
    public:

    const ast_visitor_filter &
    filter() const noexcept;

    ast_visitor_filter &
    filter() noexcept;

    bool
    print_ast() const noexcept;

    void
    print_ast(bool print) noexcept;

    bool
    graph_raw() const noexcept;

    void
    graph_raw(bool graph_raw) noexcept;

    const std::optional<CXCursorKind> &
    ancestor_match() const noexcept;

    void
    ancestor_match(CXCursorKind) noexcept;

    private:

    ast_visitor_filter _filter;
    bool _print_ast = false;
    bool _graph_raw = false;
    std::optional<CXCursorKind> _ancestor_match;
};

const ast_visitor_filter &
ast_visitor_policy::filter() const noexcept
{
    return this->_filter;
}

ast_visitor_filter &
ast_visitor_policy::filter() noexcept
{
    return this->_filter;
}

bool
ast_visitor_policy::print_ast() const noexcept
{
    return this->_print_ast;
}

void
ast_visitor_policy::print_ast(bool print) noexcept
{
    this->_print_ast = print;
}

bool
ast_visitor_policy::graph_raw() const noexcept
{
    return this->_graph_raw;
}

void
ast_visitor_policy::graph_raw(bool graph_raw) noexcept
{
    this->_graph_raw = graph_raw;
}

const std::optional<CXCursorKind> &
ast_visitor_policy::ancestor_match() const noexcept
{
    return this->_ancestor_match;
}

void
ast_visitor_policy::ancestor_match(CXCursorKind kind) noexcept
{
    this->_ancestor_match = kind;
}

void
function_labels(CXCursor cursor,
                std::string * label,
                std::string * decl_label,
                std::string * def_label)
{
    const CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    switch (cursor_kind)
    {
        case CXCursor_FunctionDecl:
        {
            if (label)
            {
                *label = "Function";
            }
            if (decl_label)
            {
                *decl_label = "FunctionDeclaration";
            }
            if (def_label)
            {
                *def_label = "FunctionDefinition";
            }

            break;
        }
        case CXCursor_FunctionTemplate:
        {
            CXCursor semantic_parent = clang_getCursorSemanticParent(cursor);
            if (!clang_Cursor_isNull(semantic_parent))
            {
                const auto parent_kind = clang_getCursorKind(semantic_parent);
                switch(parent_kind)
                {
                    case CXCursor_ClassDecl:
                    {
                        if (label)
                        {
                            *label = "MemberFunction";
                        }
                        if (decl_label)
                        {
                            *decl_label = "MemberFunctionDeclaration";
                        }
                        if (def_label)
                        {
                            *def_label = "MemberFunctionDefinition";
                        }

                        break;
                    }
                    case CXCursor_ClassTemplate:
                    {
                        if (label)
                        {
                            *label = "MemberFunction";
                        }
                        if (decl_label)
                        {
                            *decl_label = "MemberFunctionDeclaration";
                        }
                        if (def_label)
                        {
                            *def_label = "MemberFunctionDefinition";
                        }

                        break;
                    }
                    default:
                    {
                        if (label)
                        {
                            *label = "Function";
                        }
                        if (decl_label)
                        {
                            *decl_label = "FunctionDeclaration";
                        }
                        if (def_label)
                        {
                            *def_label = "FunctionDefinition";
                        }

                        break;
                    }
                }
            }
            else
            {
                if (label)
                {
                    *label = "Function";
                }
                if (decl_label)
                {
                    *decl_label = "FunctionDeclaration";
                }
                if (def_label)
                {
                    *def_label = "FunctionDefinition";
                }

            }

            break;
        }
        case CXCursor_CXXMethod:
        {
            if (label)
            {
                *label = "MemberFunction";
            }
            if (decl_label)
            {
                *decl_label = "MemberFunctionDeclaration";
            }
            if (def_label)
            {
                *def_label = "MemberFunctionDefinition";
            }

            break;
        }
        case CXCursor_Constructor:
        {
            if (label)
            {
                *label = "Constructor";
            }
            if (decl_label)
            {
                *decl_label = "ConstructorDeclaration";
            }
            if (def_label)
            {
                *def_label = "ConstructorDefinition";
            }

            break;
        }
        case CXCursor_Destructor:
        {
            if (label)
            {
                *label = "Destructor";
            }
            if (decl_label)
            {
                *decl_label = "DestructorDeclaration";
            }
            if (def_label)
            {
                *def_label = "DestructorDefinition";
            }

            break;
        }
        default:
        {
            throw std::logic_error("not a function");
        }
    };
}

class ast_visitor
{
    public:

    explicit
    ast_visitor(std::reference_wrapper<mg::Client> client,
                std::optional<std::reference_wrapper<const ast_visitor_policy>> policy = std::nullopt);

    static
    CXChildVisitResult
    graph (CXCursor cursor, CXCursor parent_cursor, CXClientData client_data);

    std::string
    fully_qualified_namespace() const;

    private:

    CXChildVisitResult
    graph (CXCursor cursor,
           CXCursor parent_cursor);

    CXChildVisitResult
    graph_raw(CXCursor cursor,
              CXCursor parent_cursor);

    bool
    graph_parent(CXCursor cursor,
                 CXCursor parent_cursor);

    bool
    graph_namespace(vector_sentry<name_decl> & sentry,
                    CXCursor cursor,
                    CXCursor parent_cursor);

    bool
    graph_function_decl(vector_sentry<function_decl> & function_def_sentry,
                        CXCursor cursor,
                        CXCursor parent_cursor);

    bool
    graph_function_call(CXCursor cursor, CXCursor parent_cursor);

    bool
    graph_class_decl(vector_sentry<name_decl> & name_sentry,
                     CXCursor cursor,
                     CXCursor parent_cursor);

    bool
    graph_base_class_specifier(CXCursor, CXCursor parent_cursor);

    /** Performs the following actions
     *  - creates the member function declaration
     *  - creates the member function if it doesn't already exist
     *  - 
     */
    bool
    graph_member_function_decl(vector_sentry<function_decl> & function_def_sentry,
                               CXCursor cursor,
                               CXCursor parent_cursor);

    bool
    graph_constructor(vector_sentry<function_decl> & ctor_def_sentry,
                      CXCursor cursor,
                      CXCursor parent_cursor);

    bool
    graph_destructor(vector_sentry<function_decl> & dtor_def_sentry,
                      CXCursor cursor,
                      CXCursor parent_cursor);

    bool
    graph_function(CXCursor cursor,
                   CXCursor parent_cursor,
                   vector_sentry<function_decl> & function_def_sentry,
                   bool & created);

    std::vector<name_decl> _names;
    mg::Client * const _mgclient = nullptr;
    ast_visitor_policy const * _policy = nullptr;
    std::vector<function_decl> _function_definitions;
    std::vector<cursor_location> _ancestor_matches;

    ngmg::cypher::property_set _child_prop_set = {};
    unsigned int _level = 0;
};

ast_visitor::ast_visitor(std::reference_wrapper<mg::Client> mgclient,
                         std::optional<std::reference_wrapper<const ast_visitor_policy>> policy):
    _mgclient(&mgclient.get())
{
    if (policy)
    {
        this->_policy = &policy->get();
    }
}

CXChildVisitResult
ast_visitor::graph(CXCursor cursor, CXCursor parent_cursor, CXClientData client_data)
{
    ast_visitor & visitor = *(reinterpret_cast<ast_visitor *>(client_data));
    visitor.graph(cursor, parent_cursor);
    return CXChildVisit_Continue;
}

CXChildVisitResult
ast_visitor::graph_raw(CXCursor cursor, CXCursor parent_cursor)
{
    static raw_node node;

    node.clear_sets();
    node.fill_match_props(cursor);
    const bool node_exists = ngmg::cypher::node_return(*this->_mgclient,
                                                       node.match_property_tuple(),
                                                       std::tie(node.visited_property));

    if (node_exists && node.visited_property.value())
    {
        // cursor is already graphed and visited
        return CXChildVisit_Continue;
    }

    node.visited_property.value(true);

    if (!node_exists)
    {
        // node doesn't exist, so create it
        node.fill_non_match_props(cursor);
        ngmg::cypher::create_node(*this->_mgclient,
                                  node.label_set,
                                  node.property_tuple(),
                                  node.property_set);
    }
    else
    {
        ngmg::cypher::match_set(*this->_mgclient,
                                node.match_property_tuple(),
                                std::tie(node.visited_property));
    }

    {
        // Handle parents

        {
            static raw_node parent_node;
            static const ngmg::cypher::label parent_label("PARENT");

            parent_node.clear_sets();
            parent_node.fill_match_props(parent_cursor);
            const bool parent_exists =
                ngmg::cypher::node_exists(*this->_mgclient,
                                          parent_node.match_property_tuple());

            if (!parent_exists)
            {
                parent_node.visited_property.value(false);
                parent_node.fill_non_match_props(parent_cursor);
                ngmg::cypher::create_node(*this->_mgclient,
                                          parent_node.label_set,
                                          parent_node.property_tuple(),
                                          parent_node.property_set);
            }

            ngmg::cypher::merge_relate(*this->_mgclient,
                                       parent_label,
                                       node.match_property_tuple(),
                                       parent_node.match_property_tuple());
        }

        {
            static raw_node lexical_parent_node;
            static const ngmg::cypher::label parent_label("LEXICAL_PARENT");

            CXCursor lexical_parent_cursor = clang_getCursorLexicalParent(cursor);
            if (!clang_Cursor_isNull(lexical_parent_cursor))
            {
                lexical_parent_node.clear_sets();
                lexical_parent_node.fill_match_props(lexical_parent_cursor);
                const bool lexical_parent_exists =
                    ngmg::cypher::node_exists(*this->_mgclient,
                                              lexical_parent_node.match_property_tuple());


                if (!lexical_parent_exists)
                {
                    // add lexical parent
                    lexical_parent_node.visited_property.value(false);
                    lexical_parent_node.fill_non_match_props(lexical_parent_cursor);
                    ngmg::cypher::create_node(*this->_mgclient,
                                              lexical_parent_node.label_set,
                                              lexical_parent_node.property_tuple(),
                                              lexical_parent_node.property_set);
                }

                ngmg::cypher::merge_relate(*this->_mgclient,
                                           parent_label,
                                           node.match_property_tuple(),
                                           lexical_parent_node.match_property_tuple());
            }
        }

        {
            static raw_node semantic_parent_node;
            static const ngmg::cypher::label parent_label("SEMANTIC_PARENT");

            CXCursor semantic_parent_cursor = clang_getCursorSemanticParent(cursor);
            if (!clang_Cursor_isNull(semantic_parent_cursor))
            {
                semantic_parent_node.clear_sets();
                semantic_parent_node.fill_match_props(semantic_parent_cursor);
                const bool semantic_parent_exists =
                    ngmg::cypher::node_exists(*this->_mgclient,
                                              semantic_parent_node.match_property_tuple());

                if (!semantic_parent_exists)
                {
                    // add semantic parent
                    semantic_parent_node.visited_property.value(false);
                    semantic_parent_node.fill_non_match_props(semantic_parent_cursor);
                    ngmg::cypher::create_node(*this->_mgclient,
                                              semantic_parent_node.label_set,
                                              semantic_parent_node.property_tuple(),
                                              semantic_parent_node.property_set);
                }

                ngmg::cypher::merge_relate(*this->_mgclient,
                                           parent_label,
                                           node.match_property_tuple(),
                                           semantic_parent_node.match_property_tuple());
            }
        }
    }

    // handle referenced cursor

    CXCursor ref_cursor = clang_getCursorReferenced(cursor);
    if (!clang_Cursor_isNull(ref_cursor) && !clang_equalCursors(cursor, ref_cursor))
    {
        static raw_node ref_node;

        ref_node.clear_sets();
        ref_node.fill_match_props(ref_cursor);

        const bool ref_node_exists = ngmg::cypher::node_exists(*this->_mgclient,
                                                               ref_node.match_property_tuple());

        if (!ref_node_exists)
        {
            ref_node.fill_non_match_props(cursor);
            ref_node.visited_property.value(false);

            // Implicit template function instantiations generate
            // their own FunctionDecl cursors that for some reason are
            // not visited.  So, when a match is performed nothing is
            // returned because no cursor with the kind and location
            // has been visited.

            // Also, cursors can be referenced before they have been
            // visited.

            // To handle these two cases, when a referenced cursor's
            // node does not exist, we create its node here with
            // 'visited' set to false.  Then if that node is
            // subsequently visited, we can set its 'visited' property
            // true and create its parent relationship.

            // In the end we should be able to detect what nodes are
            // never visited by matching all nodes whose 'visited'
            // property is set to false.

            ngmg::cypher::create_node(*this->_mgclient,
                                      ref_node.label_set,
                                      ref_node.property_tuple(),
                                      ref_node.property_set);
        }

        static const ngmg::cypher::label references_label("REFERENCES");
        ngmg::cypher::merge_relate(*this->_mgclient,
                                   references_label,
                                   node.match_property_tuple(),
                                   ref_node.match_property_tuple());
    }


    clang_visitChildren(cursor, &ast_visitor::graph, this);
    return CXChildVisit_Continue;
}

CXChildVisitResult
ast_visitor::graph(CXCursor cursor, CXCursor parent_cursor)
{
    if(clang_Location_isInSystemHeader( clang_getCursorLocation( cursor ) ) != 0 )
    {
        return CXChildVisit_Continue;
    }

    const CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    vector_sentry<cursor_location> ancestor_matches_sentry(std::ref(this->_ancestor_matches));

    if (this->_policy)
    {
        const cursor_location cursor_loc = cursor_location(cursor);

        if (this->_policy->print_ast())
        {
            ::print_cursor(cursor, parent_cursor, this->_level);
        }

        if (this->_policy->ancestor_match())
        {
            if (this->_policy->ancestor_match().value() != cursor_kind &&
                this->_ancestor_matches.empty())
            {
                // Not an ancesestor match and the ancestor has not
                // been found yet, so parse deeper.
                clang_visitChildren(cursor, &ast_visitor::graph, this);
                return CXChildVisit_Continue;
            }

            // At this point either the current cursor is the kind
            // we're looking for, or we already have an ancestor match
            // so proceed with graphing.

            if (cursor_kind == this->_policy->ancestor_match().value())
            {
                ancestor_matches_sentry.push(cursor_loc);
            }
        }

        if (this->_policy->graph_raw())
        {
            return this->graph_raw(cursor, parent_cursor);
        }
    }

    vector_sentry<name_decl> name_sentry(std::ref(this->_names));
    vector_sentry<function_decl> function_def_sentry(std::ref(this->_function_definitions));

    switch (cursor_kind)
    {
        case CXCursor_Namespace:
        {
            if (!this->graph_namespace(name_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_FunctionDecl:
        {
            if (!this->graph_function_decl(function_def_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_FunctionTemplate:
        {
            if (!this->graph_function_decl(function_def_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }
            break;
        }
        case CXCursor_CallExpr:
        {
            if (!this->graph_function_call(cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_ClassDecl:
        {
            if (!this->graph_class_decl(name_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_ClassTemplate:
        {
            if (!this->graph_class_decl(name_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }
            break;
        }
        case CXCursor_CXXMethod:
        {
            if (!this->graph_member_function_decl(function_def_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_Constructor:
        {
            if (!this->graph_constructor(function_def_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_Destructor:
        {
            if (!this->graph_destructor(function_def_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_CXXBaseSpecifier:
        {
            if (!this->graph_base_class_specifier(cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        default:
        {
            break;
        }
    }

    ++this->_level;
    clang_visitChildren(cursor, &ast_visitor::graph, this);
    --this->_level;
    return CXChildVisit_Continue;
}

bool
ast_visitor::graph_parent(CXCursor cursor, CXCursor parent_cursor)
{
    const universal_symbol_reference_property cursor_usr {cursor};
    const auto parent_usr = [&cursor, &parent_cursor]() {
        CXCursor semantic_parent = clang_getCursorSemanticParent(cursor);
        if (clang_Cursor_isNull(semantic_parent))
        {
            return universal_symbol_reference_property {parent_cursor};
        }
        else
        {
            return universal_symbol_reference_property {semantic_parent};
        }
    }();

    if (parent_usr.prop.value().empty() || cursor_usr.prop.value().empty())
    {
        return true;
    }

    if (ngmg::cypher::relationship_exists(*this->_mgclient,
                                          has_label,
                                          parent_usr.tuple(),
                                          cursor_usr.tuple()))
    {
        return true;
    }

    ngmg::cypher::create_relate(*this->_mgclient,
                                has_label,
                                parent_usr.tuple(),
                                cursor_usr.tuple());
    return true;
}

bool
ast_visitor::graph_namespace(vector_sentry<name_decl> & name_sentry, CXCursor cursor, CXCursor parent_cursor)
{
    namespace_decl_node namespace_decl;
    namespace_decl.location.fill(cursor);

    if (ngmg::cypher::node_exists(*this->_mgclient,
                                  namespace_decl.label(),
                                  namespace_decl.location.tuple()))
    {
        return true;
    }

    name_sentry.push(name_decl{ngclang::to_string(cursor, &clang_getCursorDisplayName)});
    namespace_decl.names.fill_with_fq_name(cursor, this->fully_qualified_namespace());

    ngmg::cypher::create_node(*this->_mgclient,
                              namespace_decl.label(),
                              namespace_decl.tuple());

    namespace_node namespace_node;
    namespace_node.usr.fill(cursor);
    if (!ngmg::cypher::node_exists(*this->_mgclient,
                                   namespace_node.label(),
                                   namespace_node.usr.tuple()))
    {
        namespace_node.names.fill_with_fq_name(cursor, this->fully_qualified_namespace());
        ngmg::cypher::create_node(*this->_mgclient,
                                  namespace_node.label(),
                                  namespace_node.tuple());
    }

    ngmg::cypher::create_relate(*this->_mgclient,
                                declares_label,
                                namespace_decl.location.tuple(),
                                namespace_decl.label(),
                                namespace_node.usr.tuple(),
                                namespace_node.label());

    return this->graph_parent(cursor, parent_cursor);
}

bool
ast_visitor::graph_function(CXCursor cursor,
                            CXCursor parent_cursor,
                            vector_sentry<function_decl> & function_def_sentry,
                            bool & created)
{
    created = false;
    std::string function_label;
    std::string function_dec_label;
    std::string function_def_label;
    function_labels(cursor, &function_label, &function_dec_label, &function_def_label);

    function_node func_node {function_label};
    func_node.usr.fill(cursor);

    if (!ngmg::cypher::node_exists(*this->_mgclient,
                                   func_node.label(),
                                   func_node.usr.tuple()))
    {
        func_node.is_template.fill(cursor);
        func_node.names.fill_with_fq_namespace(cursor, this->fully_qualified_namespace());
        ngmg::cypher::create_node(*this->_mgclient,
                                  func_node.label(),
                                  func_node.tuple());
        created = true;
    }

    if (clang_isCursorDefinition(cursor))
    {
        function_decl function_def {cursor};
        function_def_sentry.push(function_def);

        function_decl_def_node func_def_node {function_def_label};
        func_def_node.location.fill(cursor);

        if (!ngmg::cypher::node_exists(*this->_mgclient,
                                       func_def_node.label(),
                                       func_def_node.location.tuple()))
        {
            func_def_node.names.fill_with_fq_namespace(cursor, this->fully_qualified_namespace());
            ngmg::cypher::create_node(*this->_mgclient,
                                      func_def_node.label(),
                                      func_def_node.tuple());

            ngmg::cypher::create_relate(*this->_mgclient,
                                        defines_label,
                                        func_def_node.location.tuple(),
                                        func_def_node.label(),
                                        func_node.usr.tuple(),
                                        func_node.label());
        }
    }
    else
    {
        function_decl_def_node func_decl_node {function_dec_label};
        func_decl_node.location.fill(cursor);

        if (!ngmg::cypher::node_exists(*this->_mgclient,
                                       func_decl_node.label(),
                                       func_decl_node.location.tuple()))
        {
            func_decl_node.names.fill_with_fq_namespace(cursor, this->fully_qualified_namespace());
            ngmg::cypher::create_node(*this->_mgclient,
                                      func_decl_node.label(),
                                      func_decl_node.tuple());

            ngmg::cypher::create_relate(*this->_mgclient,
                                        declares_label,
                                        func_decl_node.location.tuple(),
                                        func_decl_node.label(),
                                        func_node.usr.tuple(),
                                        func_node.label());
        }
    }

    return true;
}

bool
ast_visitor::graph_function_decl(vector_sentry<function_decl> & function_def_sentry,
                                 CXCursor cursor,
                                 CXCursor parent_cursor)
{
    bool created = false;
    if (!this->graph_function(cursor, parent_cursor, function_def_sentry, created))
    {
        return false;
    }

    if (!created)
    {
        // function already existed so nothing else to do
        return true;
    }

    return this->graph_parent(cursor, parent_cursor);
}

bool
ast_visitor::graph_function_call(CXCursor cursor, CXCursor parent)
{
    if (this->_function_definitions.empty())
    {
        return true;
    }

    CXCursor callee_cursor = clang_getCursorReferenced(cursor);

    if (clang_Cursor_isNull(callee_cursor))
    {
        return true;
    }

    const location_properties cursor_loc {cursor};
    const universal_symbol_reference_property callee_usr {callee_cursor};
    universal_symbol_reference_property caller_usr;
    caller_usr.prop = this->_function_definitions.back().universal_symbol_reference();

    if (ngmg::cypher::relationship_exists(*this->_mgclient,
                                          calls_label,
                                          ngmg::cypher::relationship_type::directed,
                                          cursor_loc.tuple()))
    {
        return true;
    }

    ngmg::cypher::create_relate(*this->_mgclient,
                                calls_label,
                                caller_usr.tuple(),
                                callee_usr.tuple(),
                                ngmg::cypher::relationship_type::directed,
                                cursor_loc.tuple());

    return true;
}

bool
ast_visitor::graph_class_decl(vector_sentry<name_decl> & name_sentry, CXCursor cursor, CXCursor parent_cursor)
{
    class_decl_node class_decl;
    class_decl.location.fill(cursor);
    if (ngmg::cypher::node_exists(*this->_mgclient,
                                  class_decl.label(),
                                  class_decl.location.tuple()))
    {
        return true;
    }

    name_sentry.push(name_decl{ngclang::to_string(cursor, &clang_getCursorDisplayName)});
    class_decl.names.fill_with_fq_name(cursor, this->fully_qualified_namespace());

    ngmg::cypher::create_node(*this->_mgclient,
                              class_decl.label(),
                              class_decl.tuple());

    class_node class_node;
    class_node.usr.fill(cursor);

    if (!ngmg::cypher::node_exists(*this->_mgclient,
                                   class_node.label(),
                                   class_node.usr.tuple()))
    {
        class_node.names.fill_with_fq_name(cursor, this->fully_qualified_namespace());
        class_node.is_template.fill(cursor);
        ngmg::cypher::create_node(*this->_mgclient,
                                  class_node.label(),
                                  class_node.tuple());
    }

    ngmg::cypher::create_relate(*this->_mgclient,
                                declares_label,
                                class_decl.location.tuple(),
                                class_decl.label(),
                                class_node.usr.tuple(),
                                class_node.label());

    return this->graph_parent(cursor, parent_cursor);
}

bool
ast_visitor::graph_base_class_specifier(CXCursor cursor, CXCursor parent_cursor)
{
    CXCursor base_cursor = clang_getCursorReferenced(cursor);
    const universal_symbol_reference_property base_usr {base_cursor};
    const universal_symbol_reference_property child_usr {parent_cursor};

    if (ngmg::cypher::relationship_exists(*this->_mgclient,
                                          inherits_label,
                                          child_usr.tuple(),
                                          class_node::label(),
                                          base_usr.tuple(),
                                          class_node::label()))
    {
        return true;
    }

    ngmg::cypher::create_relate(*this->_mgclient,
                                inherits_label,
                                child_usr.tuple(),
                                class_node::label(),
                                base_usr.tuple(),
                                class_node::label());

    return true;
}

bool
ast_visitor::graph_member_function_decl(vector_sentry<function_decl> & function_def_sentry,
                                        CXCursor cursor,
                                        CXCursor parent_cursor)
{
    bool created = false;
    if (!this->graph_function(cursor, parent_cursor, function_def_sentry, created))
    {
        return false;
    }

    if (!created)
    {
        return true;
    }

    if (!this->graph_parent(cursor, parent_cursor))
    {
        return false;
    }

    // Virtual function overrides
    ngclang::overridden_cursors_t overrides;
    unsigned num_overrides;
    clang_getOverriddenCursors(cursor, &overrides.get(), &num_overrides);

    const universal_symbol_reference_property cursor_usr {cursor};
    const ngmg::cypher::label member_func_label {"MemberFunction"};

    for(unsigned i = 0; i < num_overrides; ++i)
    {
        const universal_symbol_reference_property override_usr {overrides.get()[i]};
        ngmg::cypher::create_relate(*this->_mgclient,
                                    overrides_label,
                                    cursor_usr.tuple(),
                                    member_func_label,
                                    override_usr.tuple(),
                                    member_func_label);
    }

    return true;
}

bool
ast_visitor::graph_constructor(vector_sentry<function_decl> & function_def_sentry,
                               CXCursor cursor,
                               CXCursor parent_cursor)
{
    bool created = false;
    if (!this->graph_function(cursor, parent_cursor, function_def_sentry, created))
    {
        return false;
    }

    if (!created)
    {
        return true;
    }

    return this->graph_parent(cursor, parent_cursor);
}

bool
ast_visitor::graph_destructor(vector_sentry<function_decl> & function_def_sentry,
                              CXCursor cursor,
                              CXCursor parent_cursor)
{
    bool created = false;
    if (!this->graph_function(cursor, parent_cursor, function_def_sentry, created))
    {
        return false;
    }

    if (!created)
    {
        return true;
    }

    return this->graph_parent(cursor, parent_cursor);
}

std::string
ast_visitor::fully_qualified_namespace() const
{
    std::string name;
    if (this->_names.empty())
    {
        return name;
    }

    name = this->_names.front().name();
    for (auto i = this->_names.cbegin() + 1;
         i != this->_names.cend();
         ++i)
    {
        name += "::";
        name += i->name();
    }

    return name;
}

class compile_command
{
    public:

    explicit
    compile_command(CXCompileCommand);
    compile_command(const std::string & file);

    ~compile_command();

    compile_command(const compile_command &) = delete;
    compile_command& operator = (const compile_command &) = delete;

    char * const *
    array() const noexcept;

    size_t
    size() const noexcept;

    bool
    empty() const noexcept;

    private:
    std::vector<char *> _commands;
};

compile_command::compile_command(const std::string & file)
{
    this->_commands.push_back(new char[file.size() + 1]);
    std::fill(this->_commands.back(), this->_commands.back() + file.size() + 1, 0);
    file.copy(this->_commands.back(), file.size());
}

compile_command::compile_command(CXCompileCommand cx_compile_command)
{
    const unsigned sizeof_args = clang_CompileCommand_getNumArgs(cx_compile_command);
    this->_commands.resize(sizeof_args);

    for(unsigned i = 0; i < sizeof_args; ++i)
    {
        ngclang::string_t cxstring(clang_CompileCommand_getArg(cx_compile_command, i));
        char const * const command = clang_getCString(cxstring.get());
        const size_t sizeof_command = std::strlen(command);
        this->_commands[i] = new char[sizeof_command + 1];
        std::fill(this->_commands[i], this->_commands[i] + sizeof_command + 1, 0);
        std::copy(command, command + sizeof_command, this->_commands[i]);
    }
}

compile_command::~compile_command()
{
    for (auto c : this->_commands)
    {
        delete [] c;
    }
}

char * const *
compile_command::array() const noexcept
{
    
    return this->_commands.empty() ? nullptr : &this->_commands.front();
}

size_t
compile_command::size() const noexcept
{
    return this->_commands.size();
}

bool
compile_command::empty() const noexcept
{
    return this->_commands.empty();
}

void parse_file(CXIndex index, const std::string & file, mg::Client & client)
{
    ngclang::translation_unit_t unit = 
        clang_parseTranslationUnit(
            index, // clang index
            file.c_str(), // path
            nullptr, // command line args
            0, // number of command line args
            nullptr, // clang unsaved files
            0, // number of unsaved files
            CXTranslationUnit_None);

    CXCursor cursor = clang_getTranslationUnitCursor(unit.get());

    ast_visitor visitor(std::ref(client));
    clang_visitChildren(cursor, &ast_visitor::graph, &visitor);
}

void parse_compile_command(CXIndex index,
                           CXCompileCommand cx_compile_command,
                           mg::Client & client,
                           const ast_visitor_policy & policy)
{
    compile_command commands (cx_compile_command);
    ngclang::translation_unit_t unit;
    const CXErrorCode error =
        clang_parseTranslationUnit2FullArgv(
            index,
            nullptr,
            commands.array(),
            commands.size(),
            nullptr,
            0,
            CXTranslationUnit_KeepGoing | CXTranslationUnit_IgnoreNonErrorsFromIncludedFiles,
            &unit.get());

    if (error != CXError_Success)
    {
        std::cerr << "error parsing file\n";
        return;
    }

    CXCursor cursor = clang_getTranslationUnitCursor(unit.get());

    ast_visitor visitor(std::ref(client), std::ref(policy));
    clang_visitChildren(cursor, &ast_visitor::graph, &visitor);
}

int main(int argc, char ** argv)
{
    std::string build_dir;
    std::string file_to_parse;

    static const struct option long_options [] = {
        {"src-dir", required_argument, nullptr, 0},
        {"src-tree", required_argument, nullptr, 1},
        {"src-file", required_argument, nullptr, 2},
        {"raw", no_argument, nullptr,3},
        {"ancestor", required_argument, nullptr, 4},
        {0,0,0,0}
    };

    ast_visitor_policy policy;
    int option_index;
    for(;;)
    {
        switch(::getopt_long(argc, argv, "s:t:d:f:phr",
                             long_options, &option_index))
        {
            case 'd':
            {
                build_dir = optarg;
                continue;
            }
            case 'f':
            {
                file_to_parse = optarg;
                continue;
            }
            case 'p':
            {
                policy.print_ast(true);
                continue;
            }
            case 'h':
            {
                std::cerr << help_text << std::endl;
                return 0;
            }
            case 's':
            {
                policy.filter().push_back_src_dir(optarg);
                continue;
            }
            case 0:
            {
                policy.filter().push_back_src_dir(optarg);
                continue;
            }
            case 't':
            {
                policy.filter().push_back_src_tree(optarg);
                continue;
            }
            case 1:
            {
                policy.filter().push_back_src_tree(optarg);
                continue;
            }
            case 2:
            {
                policy.filter().push_back_src_file(optarg);
                continue;
            }
            case 3:
            {
                policy.graph_raw(true);
                continue;
            }
            case 4:
            {
                const std::string ancestor = optarg;
                if (ancestor == "CallExpr")
                {
                    policy.ancestor_match(CXCursor_CallExpr);
                    continue;
                }
                else
                {
                    std::cerr << "unknown ancestor kind\n";
                }
            }
            case -1:
            {
                break;
            }
        }

        break;
    }
    
    if (file_to_parse.empty() && build_dir.empty())
    {
        std::cerr << "missing file or build directory to parse\n";
        return 3;
    }

    memgraph_init mg;

    mg::Client::Params params;
    params.host = "127.0.0.1";
    params.port = 7687;
    auto client = mg::Client::Connect(params);
    if (!client)
    {
        std::cerr << "failed to connect to db\n";
        return 2;
    }

    client->Execute("MATCH (n) DETACH DELETE n;");
    client->DiscardAll();

    client->Execute("CREATE INDEX ON :Namespace(universal_symbol_reference);");
    client->DiscardAll();

    client->Execute("CREATE INDEX ON :Class(universal_symbol_reference);");
    client->DiscardAll();

    client->Execute("CREATE INDEX ON :Function(universal_symbol_reference);");
    client->DiscardAll();

    client->Execute("CREATE INDEX ON :MemberFunction(universal_symbol_reference);");
    client->DiscardAll();

    client->Execute("CREATE INDEX ON :Constructor(universal_symbol_reference);");
    client->DiscardAll();

    client->Execute("CREATE INDEX ON :Destructor(universal_symbol_reference);");
    client->DiscardAll();

    ngclang::object<CXIndex, ngclang::dispose_index> index = clang_createIndex(0,1);

    if (!file_to_parse.empty())
    {
        parse_file(index.get(), file_to_parse, *client);
    }
    else if (!build_dir.empty())
    {
        CXCompilationDatabase_Error error_code = CXCompilationDatabase_NoError;
        ngclang::object<CXCompilationDatabase, ngclang::dispose_compilation_database>
            compilation_database =
            clang_CompilationDatabase_fromDirectory(build_dir.c_str(),
                                                    &error_code);

        if (error_code != CXCompilationDatabase_NoError)
        {
            std::cerr << "error creating compilation database" << std::endl;
            return 3;
        }

        ngclang::object<CXCompileCommands, ngclang::dispose_compile_commands> compile_commands =
            clang_CompilationDatabase_getAllCompileCommands(compilation_database.get());

        const unsigned sizeof_compile_commands = clang_CompileCommands_getSize(compile_commands.get());

        for (unsigned i = 0; i < sizeof_compile_commands; ++i)
        {
            CXCompileCommand compile_command =
                clang_CompileCommands_getCommand(compile_commands.get(), i);

            const std::filesystem::path file_path(ngclang::to_string(clang_CompileCommand_getFilename(compile_command)));
            if (!policy.filter().parse_file(file_path))
            {
                // The file does not match the specified filter so
                // don't parse.
                continue;
            }

            std::cout << "parsing: " << file_path << std::endl;
            parse_compile_command(index.get(), compile_command, *client, policy);
        }
    }

    return 0;
}
