#include <algorithm>
#include <array>
#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <getopt.h>
#include "help.hpp"
#include <iostream>
#include <memory>
#include "ngclang.hpp"
#include <mgclient.hpp>
#include <optional>
#include <sstream>
#include "statement_executor.hpp"
#include <string>
#include <unistd.h>
#include <vector>

class field_name
{
    public:

    virtual
    std::string
    to_string() const = 0;
};

template <std::size_t N>
struct fixed_string
{
    constexpr fixed_string(char const (&s) [N]) {
        std::copy(s, s + N, this->string.begin());
    }

    std::array<char, N> string;
};

template <fixed_string str>
struct field_name_template: public field_name
{
    public:
    
    std::string
    to_string() const override
    {
        return std::string(str.string.data());
    }
};

field_name_template<"field"> f;


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
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

class cursor_location
{
    public:

    explicit
    cursor_location(CXCursor c);

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

CXChildVisitResult
printing_visitor(CXCursor c, CXCursor parent, CXClientData client_data)
{
    const CXCursorKind cursor_kind = clang_getCursorKind(c);
    const CXType cursor_type = clang_getCursorType(c);
    const CXSourceLocation location = clang_getCursorLocation(c);
    CXCursor ref = clang_getCursorReferenced(c);
    const std::string usr = ngclang::to_string(c, &clang_getCursorUSR);
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

    const unsigned int level = *(reinterpret_cast<unsigned int * const>(client_data));
    const std::string cursor_indent = std::string(level, '-') + '>';
    const std::string cursor_property_indent = std::string(level + 1, ' ') + '|';
    std::cout << cursor_indent
              << clang_getCursorKindSpelling(cursor_kind) << std::endl
              << cursor_property_indent << " type "
              << '(' << clang_getTypeSpelling(cursor_type) << ')' << std::endl
              << cursor_property_indent << " usr "
              << '(' << usr << ')' << std::endl
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

    if (!clang_Cursor_isNull(ref))
    {
        std::cout << cursor_property_indent << " ref "
                  << '(' << clang_getTypeSpelling(clang_getCursorType(ref)) << ')' << std::endl;
    }


    unsigned int next_level = level + 1;
    clang_visitChildren(c, &printing_visitor, &next_level);
    return CXChildVisit_Continue;
}

class namespace_decl
{
    public:
    namespace_decl(const std::string & name,
                   const cursor_location location);

    std::string
    name() const;

    unsigned int
    line() const;

    unsigned int
    column() const;

    private:

    std::string _name;
    cursor_location _location;
};

namespace_decl::namespace_decl(const std::string & name,
                               const cursor_location location):
    _name(name),
    _location(location)
{}

std::string
namespace_decl::name() const
{
    return this->_name;
}

unsigned int
namespace_decl::line() const
{
    return this->_location.line();
}

unsigned int
namespace_decl::column() const
{
    return this->_location.column();
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
class stack_sentry
{
    public:

    stack_sentry(std::reference_wrapper<std::vector<T>> stack);
    ~stack_sentry();

    void
    push(const T);

    std::vector<T> * const _stack;
    bool _armed = false;
};

template <class T>
stack_sentry<T>::stack_sentry(std::reference_wrapper<std::vector<T>> stack):
    _stack(&stack.get())
{}

template <class T>
stack_sentry<T>::~stack_sentry()
{
    if (this->_armed)
    {
        this->_stack->pop_back();
    }
}

template <class T>
void
stack_sentry<T>::push(const T t)
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
    parse_file(const std::filesystem::path & file) const;

    void
    push_back_src_dir(const std::filesystem::path & src_dir);

    void
    push_back_src_tree(const std::filesystem::path & src_tree);

    private:
    std::vector<std::filesystem::path> _src_dirs;
    std::vector<std::filesystem::path> _src_trees;
};

bool
ast_visitor_filter::is_src_dir(const std::filesystem::path & file) const
{
    if (this->_src_dirs.empty())
    {
        // no source dir filter, so all sources are valid
        return true;
    }

    std::filesystem::path src_dir = file;
    src_dir.remove_filename();

    auto is_src_dir = [&src_dir] (const std::filesystem::path & p) {return std::filesystem::equivalent(src_dir, p);};
    return (std::find_if(this->_src_dirs.cbegin(), this->_src_dirs.cend(), is_src_dir) != this->_src_dirs.cend());
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

bool
ast_visitor_filter::parse_file(const std::filesystem::path & file) const
{
    if (!this->_src_trees.empty())
    {
        return this->in_src_tree(file);
    }

    if (this->is_src_dir(file))
    {
        return true;
    }

    return false;
}

class ast_visitor
{
    public:

    explicit
    ast_visitor(std::reference_wrapper<mg::Client> client,
                std::optional<std::reference_wrapper<const ast_visitor_filter>> filter = std::nullopt);

    static
    CXChildVisitResult
    graph (CXCursor cursor, CXCursor parent_cursor, CXClientData client_data);

    std::string
    fully_qualified_namespace() const;

    namespace_decl const *
    parent_namespace() const;

    private:

    CXChildVisitResult
    graph (CXCursor cursor,
           CXCursor parent_cursor);

    bool
    graph_namespace(stack_sentry<namespace_decl> & sentry,
                    CXCursor cursor,
                    CXCursor parent_cursor);

    bool
    graph_function_decl(stack_sentry<function_decl> & function_def_sentry,
                        CXCursor cursor,
                        CXCursor parent_cursor);

    bool
    graph_decl_ref_expr(CXCursor cursor, CXCursor parent_cursor);

    bool
    graph_function_call(CXCursor cursor, CXCursor parent_cursor);

    bool
    graph_class_decl(CXCursor cursor, CXCursor parent_cursor);

    bool
    graph_base_class_specifier(CXCursor, CXCursor parent_cursor);

    /** Performs the following actions
     *  - creates the member function declaration
     *  - creates the member function if it doesn't already exist
     *  - 
     */
    bool
    graph_member_function_decl(stack_sentry<function_decl> & function_def_sentry,
                               CXCursor cursor,
                               CXCursor parent_cursor);

    bool
    graph_constructor(stack_sentry<function_decl> & ctor_def_sentry,
                      CXCursor cursor,
                      CXCursor parent_cursor);

    bool
    graph_destructor(stack_sentry<function_decl> & dtor_def_sentry,
                      CXCursor cursor,
                      CXCursor parent_cursor);

    bool
    graph_member_ref_expr(CXCursor cursor, CXCursor parent_cursor);

    bool
    graph_member_function_call(CXCursor cursor, CXCursor parent_cursor);

    bool
    edge_exists(const std::string & label,
                const cursor_location & location);

    bool
    edge_exists(const std::string & src_label,
                const ngclang::universal_symbol_reference & src,
                const std::string & dest_label,
                const ngclang::universal_symbol_reference & dst,
                const std::string & edge_label);

    bool
    node_exists_by_location(const std::string & label,
                            const cursor_location & location);

    bool
    node_exists_by_usr(const std::string & label,
                       const std::string & universal_symbol_reference);


    std::vector<namespace_decl> _namespaces;
    mg::Client * const _mgclient = nullptr;
    ast_visitor_filter const * _filter = nullptr;
    std::vector<function_decl> _function_definitions;
    std::vector<cursor_location> _call_expressions;
};

ast_visitor::ast_visitor(std::reference_wrapper<mg::Client> mgclient,
                         std::optional<std::reference_wrapper<const ast_visitor_filter>> filter):
    _mgclient(&mgclient.get())
{
    if (filter)
    {
        this->_filter = &filter->get();
    }
}

namespace_decl const *
ast_visitor::parent_namespace() const
{
    if (this->_namespaces.empty())
    {
        return nullptr;
    }

    return &this->_namespaces.back();
}

CXChildVisitResult
ast_visitor::graph(CXCursor cursor, CXCursor parent_cursor, CXClientData client_data)
{
    ast_visitor & visitor = *(reinterpret_cast<ast_visitor *>(client_data));
    visitor.graph(cursor, parent_cursor);
    return CXChildVisit_Continue;
}

CXChildVisitResult
ast_visitor::graph(CXCursor cursor, CXCursor parent_cursor)
{
    if(clang_Location_isInSystemHeader( clang_getCursorLocation( cursor ) ) != 0 )
    {
        return CXChildVisit_Continue;
    }

    //print_cursor(cursor, parent_cursor, this->_level);

    if (this->_filter)
    {
        const cursor_location cursor_loc = cursor_location(cursor);
        if (!this->_filter->parse_file(cursor_loc.file()))
        {
            return CXChildVisit_Continue;
        }
    }

    stack_sentry<namespace_decl> namespace_sentry(std::ref(this->_namespaces));
    stack_sentry<function_decl> function_def_sentry(std::ref(this->_function_definitions));
    stack_sentry<cursor_location> call_expr_sentry(std::ref(this->_call_expressions));

    const CXCursorKind cursor_kind = clang_getCursorKind(cursor);

    switch (cursor_kind)
    {
        case CXCursor_Namespace:
        {
            if (!this->graph_namespace(namespace_sentry, cursor, parent_cursor))
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
        case CXCursor_CallExpr:
        {
            call_expr_sentry.push(cursor_location(cursor));
            break;
        }
        case CXCursor_DeclRefExpr:
        {
            if (!this->graph_decl_ref_expr(cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_ClassDecl:
        {
            if (!this->graph_class_decl(cursor, parent_cursor))
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
        }
        case CXCursor_Destructor:
        {
            if (!this->graph_destructor(function_def_sentry, cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }
        }
        case CXCursor_CXXBaseSpecifier:
        {
            if (!this->graph_base_class_specifier(cursor, parent_cursor))
            {
                return CXChildVisit_Break;
            }

            break;
        }
        case CXCursor_MemberRefExpr:
        {
            if (!this->graph_member_ref_expr(cursor, parent_cursor))
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

    clang_visitChildren(cursor, &ast_visitor::graph, this);
    return CXChildVisit_Continue;
}

bool
ast_visitor::graph_namespace(stack_sentry<namespace_decl> & namespace_sentry, CXCursor cursor, CXCursor parent_cursor)
{
    const cursor_location cursor_loc = cursor_location(cursor);
    const std::string name = ngclang::to_string(cursor, &clang_getCursorSpelling);
    const std::string usr = ngclang::to_string(cursor, &clang_getCursorUSR);
    namespace_sentry.push(namespace_decl{name, cursor_loc});

    {
        if (this->node_exists_by_location("NamespaceDeclaration", cursor_loc))
        {
            return true;
        }

        // Create the NamespaceDeclaration
        mg::Map query_params(5);
        query_params.Insert("name", mg::Value(name));
        query_params.Insert("file", mg::Value(cursor_loc.file()));
        query_params.Insert("line", mg::Value((int) cursor_loc.line()));
        query_params.Insert("column", mg::Value(cursor_loc.column()));
        query_params.Insert("fq_name", mg::Value(this->fully_qualified_namespace()));
                        
        std::stringstream ss;
        ss << "create(:NamespaceDeclaration {"
           << "name: $name,"
           << "file: $file,"
           << "line: $line,"
           << "column: $column,"
           << "fq_name: $fq_name})";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    // Create the Namespace node if it doesn't already exist
    const bool namespace_exists = [this, &usr]() {
        mg::Map query_params(1);
        query_params.Insert("universal_symbol_reference", mg::Value(usr));

        std::stringstream ss;
        ss << "match (n:Namespace) where n.universal_symbol_reference = $universal_symbol_reference return n";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            throw std::runtime_error("error executing statement");
        }

        return static_cast<bool>(this->_mgclient->FetchOne());
    }();

    if (!namespace_exists)
    {
        mg::Map query_params(3);
        query_params.Insert("fq_name", mg::Value(this->fully_qualified_namespace()));
        query_params.Insert("name", mg::Value(name));
        query_params.Insert("universal_symbol_reference", mg::Value(usr));
                        
        std::stringstream ss;
        ss << "create(:Namespace {"
           << "name: $name,"
           << "fq_name: $fq_name,"
           << "universal_symbol_reference: $universal_symbol_reference})";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    {
        // Create NamespaceDeclaration to Namespace relationship
        mg::Map query_params(4);
        query_params.Insert("universal_symbol_reference", mg::Value(usr));
        query_params.Insert("file", mg::Value(cursor_loc.file()));
        query_params.Insert("line", mg::Value(cursor_loc.line()));
        query_params.Insert("column", mg::Value(cursor_loc.column()));

        std::stringstream ss;
        ss << "match (n:Namespace), (nd:NamespaceDeclaration) where"
           << " n.universal_symbol_reference = $universal_symbol_reference"
           << " and nd.line = $line"
           << " and nd.column = $column"
           << " and nd.file = $file"
           << " create (nd)-[:DECLARES]->(n)";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    const CXCursorKind parent_kind = clang_getCursorKind(parent_cursor);
    if (parent_kind == CXCursor_Namespace)
    {
        const std::string parent_usr = ngclang::to_string(parent_cursor, &clang_getCursorUSR);

        const cursor_location parent_loc = cursor_location(parent_cursor);

        {
            mg::Map query_params(2);
            query_params.Insert("child_usr", mg::Value(usr));
            query_params.Insert("parent_usr", mg::Value(parent_usr));

            std::stringstream ss;
            ss << "match (pn:Namespace {universal_symbol_reference: $parent_usr})-[:HAS]->(cn:Namespace {universal_symbol_reference: $child_usr}) return pn";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                throw "wtf";
                return false;
            }

            if (static_cast<bool>(this->_mgclient->FetchOne()))
            {
                return true;
            }
        }

        mg::Map query_params(2);
        query_params.Insert("parent_usr", mg::Value(parent_usr));
        query_params.Insert("child_usr", mg::Value(usr));

        std::stringstream ss;
        ss << "match (pn:Namespace), (cn:Namespace) where"
           << " pn.universal_symbol_reference = $parent_usr"
           << " and cn.universal_symbol_reference = $child_usr"
           << " create (pn)-[:HAS]->(cn)";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    return true;
}

bool
ast_visitor::graph_function_decl(stack_sentry<function_decl> & function_def_sentry,
                                 CXCursor cursor,
                                 CXCursor parent_cursor)
{
    /* This function does the following:
     * - Create a Function node if one doesn't already exists
     * - Creates either a FunctionDeclaration node or a FunctionDefinition node based
     *   on whether the cursor is a declaration or definition
     * - Create the relation ship between the FunctionDeclaration or the
     *   FunctionDefinition and the Function nodes
     * - Create the relationship between the Function and the Namespace
     */
    const cursor_location cursor_loc = cursor_location(cursor);
    const std::string name = ngclang::to_string(cursor, &clang_getCursorSpelling);
    const std::string display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);
    const std::string usr = ngclang::to_string(cursor, &clang_getCursorUSR);

    if (!this->node_exists_by_usr("Function", usr))
    {
        mg::Map query_params(4);
        query_params.Insert("fq_name", mg::Value(this->fully_qualified_namespace() + "::"+ name));
        query_params.Insert("name", mg::Value(name));
        query_params.Insert("display_name", mg::Value(display_name));
        query_params.Insert("universal_symbol_reference", mg::Value(usr));

        std::stringstream ss;
        ss << "create(:Function {"
           << "name: $display_name,"
           << "unqualified_name: $name,"
           << "fq_name: $fq_name,"
           << "universal_symbol_reference: $universal_symbol_reference})";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    const unsigned is_definition = clang_isCursorDefinition(cursor);
    if (is_definition)
    {
        function_decl function_def {cursor};
        function_def_sentry.push(function_def);

        if (!this->node_exists_by_usr("FunctionDefinition", function_def.universal_symbol_reference()))
        {
            {
                const std::string display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);
                mg::Map query_params(5);
                query_params.Insert("universal_symbol_reference", mg::Value(function_def.universal_symbol_reference()));
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value(cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));
                query_params.Insert("name", mg::Value(display_name));

                std::stringstream ss;
                ss << "create(:FunctionDefinition {"
                   << "universal_symbol_reference: $universal_symbol_reference,"
                   << "file: $file,"
                   << "line: $line,"
                   << "column: $column,"
                   << "name: $name"
                   << "})";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }

            {
                mg::Map query_params(1);
                query_params.Insert("universal_symbol_reference", mg::Value(function_def.universal_symbol_reference()));

                std::stringstream ss;
                ss << "match(f:Function), (fd:FunctionDefinition) where"
                   << " f.universal_symbol_reference = $universal_symbol_reference"
                   << " and fd.universal_symbol_reference = $universal_symbol_reference"
                   << " create (fd)-[:DEFINES]->(f)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }
    else
    {
        if (!this->node_exists_by_location("FunctionDeclaration", cursor_loc))
        {
            {
                // Create function declaration node
                mg::Map query_params(4);
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value((int) cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));
                query_params.Insert("name", mg::Value(display_name));

                std::stringstream ss;
                ss << "create(:FunctionDeclaration {"
                   << "name: $name,"
                   << "file: $file,"
                   << "line: $line,"
                   << "column: $column})";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }

            {
                // Create function declaration to function relationship
                mg::Map query_params(4);
                query_params.Insert("universal_symbol_reference", mg::Value(usr));
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value(cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));

                std::stringstream ss;
                ss << "match (f:Function), (fd:FunctionDeclaration) where"
                   << " f.universal_symbol_reference = $universal_symbol_reference"
                   << " and fd.line = $line"
                   << " and fd.column = $column"
                   << " and fd.file = $file"
                   << " create (fd)-[:DECLARES]->(f)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }

    const CXCursorKind parent_kind = clang_getCursorKind(parent_cursor);
    const std::string parent_usr = ngclang::to_string(parent_cursor, &clang_getCursorUSR);
    if (parent_kind == CXCursor_Namespace)
    {
        // Create namespace to function relationship
        const bool edge_exists = [this, &parent_usr, &usr]() {

            mg::Map query_params(2);
            query_params.Insert("ns_usr", mg::Value(parent_usr));
            query_params.Insert("f_usr", mg::Value(usr));

            std::stringstream ss;
            ss << "match (f:Function {universal_symbol_reference: $f_usr})<-[:HAS]-(n:Namespace {universal_symbol_reference: $ns_usr}) return n";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                throw std::runtime_error("failed to set function to namespace relation");
            }

            return static_cast<bool>(this->_mgclient->FetchOne());
        }();

        if (edge_exists)
        {
            return true;
        }

        mg::Map query_params(2);
        query_params.Insert("n_usr", mg::Value(parent_usr));
        query_params.Insert("f_usr", mg::Value(usr));

        std::stringstream ss;
        ss << "match (n:Namespace), (f:Function) where"
           << " n.universal_symbol_reference = $n_usr"
           << " and f.universal_symbol_reference = $f_usr"
           << " create (n)-[:HAS]->(f)";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    return true;
}

bool
ast_visitor::graph_decl_ref_expr(CXCursor cursor, CXCursor parent_cursor)
{
    if (! this->_function_definitions.empty() &&
        ! this->_call_expressions.empty())
    {
        // This is a function call from a function definition
        return this->graph_function_call(cursor, parent_cursor);
    }

    return true;
}

bool
ast_visitor::graph_function_call(CXCursor cursor, CXCursor parent)
{
    const cursor_location cursor_loc = cursor_location(cursor);
    CXCursor callee_cursor = clang_getCursorReferenced(cursor);

    const std::string callee_usr = ngclang::to_string(callee_cursor, &clang_getCursorUSR);
    const std::string caller_usr = this->_function_definitions.front().universal_symbol_reference();
    
    const std::string caller_label = this->_function_definitions.front().is_member_function() ?
        "(caller:MemberFunction)" :
        "(caller:Function)";

    if (this->edge_exists(":CALLS", cursor_loc))
    {
        return true;
    }

    mg::Map query_params(5);
    query_params.Insert("callee_usr", mg::Value(callee_usr));
    query_params.Insert("caller_usr", mg::Value(caller_usr));
    query_params.Insert("file", mg::Value(cursor_loc.file()));
    query_params.Insert("line", mg::Value(cursor_loc.line()));
    query_params.Insert("column", mg::Value(cursor_loc.column()));

    std::stringstream ss;
    ss << "match (callee:Function),"
       << caller_label
       << " where "
       << " caller.universal_symbol_reference = $caller_usr"
       << " and callee.universal_symbol_reference = $callee_usr"
       << " create (caller)-[:CALLS {file: $file, line: $line, column: $column}]->(callee)";

    ngmg::statement_executor executor(std::ref(*this->_mgclient));
    if (!executor.execute(ss.str(), query_params.AsConstMap()))
    {
        return false;
    }

    return true;
}

bool
ast_visitor::graph_class_decl(CXCursor cursor, CXCursor parent_cursor)
{
    const cursor_location cursor_loc = cursor_location(cursor);
    const std::string name = ngclang::to_string(cursor, &clang_getCursorSpelling);
    const std::string display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);
    const std::string usr = ngclang::to_string(cursor, &clang_getCursorUSR);

    {
        if (this->node_exists_by_location("ClassDeclaration", cursor_loc))
        {
            // class declaration already exists, so no need to process the rest
            return true;
        }

        // Create class declaration node
        mg::Map query_params(4);
        query_params.Insert("file", mg::Value(cursor_loc.file()));
        query_params.Insert("line", mg::Value((int) cursor_loc.line()));
        query_params.Insert("column", mg::Value(cursor_loc.column()));
        query_params.Insert("name", mg::Value(display_name));
                        
        std::stringstream ss;
        ss << "create(:ClassDeclaration {"
           << "name: $name,"
           << "file: $file,"
           << "line: $line,"
           << "column: $column})";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    {
        const bool class_exists = [this, &usr](){
            // Create class node if it does not already exist
            mg::Map query_params(1);
            query_params.Insert("universal_symbol_reference", mg::Value(usr));

            std::stringstream ss;
            ss << "match (c:Class) where c.universal_symbol_reference = $universal_symbol_reference return c";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                throw std::runtime_error("error running: " + ss.str());
            }

            return static_cast<bool>(this->_mgclient->FetchOne());
        }();

        if (!class_exists)
        {
            mg::Map query_params(4);
            query_params.Insert("fq_name", mg::Value(this->fully_qualified_namespace() + "::"+ name));
            query_params.Insert("name", mg::Value(name));
            query_params.Insert("display_name", mg::Value(display_name));
            query_params.Insert("universal_symbol_reference", mg::Value(usr));
                        
            std::stringstream ss;
            ss << "create(:Class {"
               << "name: $display_name,"
               << "unqualified_name: $name,"
               << "fq_name: $fq_name,"
               << "universal_symbol_reference: $universal_symbol_reference})";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                return false;
            }
        }
    }

    {
        // Create :Class:Declaration to :Class relationship
        mg::Map query_params(4);
        query_params.Insert("universal_symbol_reference", mg::Value(usr));
        query_params.Insert("file", mg::Value(cursor_loc.file()));
        query_params.Insert("line", mg::Value(cursor_loc.line()));
        query_params.Insert("column", mg::Value(cursor_loc.column()));

        std::stringstream ss;
        ss << "match(c:Class), (cd:ClassDeclaration) where"
           << " c.universal_symbol_reference = $universal_symbol_reference"
           << " and cd.line = $line"
           << " and cd.column = $column"
           << " and cd.file = $file"
           << " create (cd)-[:DECLARES]->(c)";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    const CXCursorKind parent_kind = clang_getCursorKind(parent_cursor);
    const std::string parent_usr = ngclang::to_string(parent_cursor, &clang_getCursorUSR);
    if (parent_kind == CXCursor_Namespace)
    {
        // Create namespace to class relationship
        const bool edge_exists = [this, &parent_usr, &usr]() {
            mg::Map query_params(2);
            query_params.Insert("ns_usr", mg::Value(parent_usr));
            query_params.Insert("c_usr", mg::Value(usr));

            std::stringstream ss;
            ss << "match (c:Class {universal_symbol_reference: $c_usr})<-[:HAS]-(n:Namespace {universal_symbol_reference: $ns_usr}) return n";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                // Relationship allready exists
                throw std::runtime_error("error running: " + ss.str());
            }

            return static_cast<bool>(this->_mgclient->FetchOne());
        }();

        if (edge_exists)
        {
            return true;
        }

        const cursor_location parent_loc = cursor_location(parent_cursor);
        mg::Map query_params(2);
        query_params.Insert("n_usr", mg::Value(parent_usr));
        query_params.Insert("c_usr", mg::Value(usr));

        std::stringstream ss;
        ss << "match (n:Namespace), (c:Class) where"
           << " n.universal_symbol_reference = $n_usr"
           << " and c.universal_symbol_reference = $c_usr"
           << " create (n)-[:HAS]->(c)";

        ngmg::statement_executor executor(std::ref(*this->_mgclient));
        if (!executor.execute(ss.str(), query_params.AsConstMap()))
        {
            return false;
        }
    }

    return true;
}

bool
ast_visitor::graph_base_class_specifier(CXCursor cursor, CXCursor parent_cursor)
{
    CXCursor base_cursor = clang_getCursorReferenced(cursor);
    const auto child_usr = ngclang::universal_symbol_reference(parent_cursor);
    const auto base_usr = ngclang::universal_symbol_reference(base_cursor);

    if(this->edge_exists("Class", child_usr, "Class", base_usr, "INHERITS"))
    {
        return true;
    }

    mg::Map query_params(2);
    query_params.Insert("base_usr", mg::Value(base_usr.string()));
    query_params.Insert("child_usr", mg::Value(child_usr.string()));

    std::stringstream ss;
    ss << "match (c:Class), (b:Class) "
       << "where "
       << "c.universal_symbol_reference = $child_usr and "
       << "b.universal_symbol_reference = $base_usr "
       << "create (c)-[:INHERITS]->(b)";

    ngmg::statement_executor executor(std::ref(*this->_mgclient));
    if (!executor.execute(ss.str(), query_params.AsConstMap()))
    {
        return false;
    }

    return true;
}

bool
ast_visitor::graph_member_function_decl(stack_sentry<function_decl> & function_def_sentry,
                                        CXCursor cursor,
                                        CXCursor parent_cursor)
{
    const cursor_location cursor_loc = cursor_location(cursor);
    const std::string cursor_display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);
    const std::string cursor_usr = ngclang::to_string(cursor, &clang_getCursorUSR);
    const std::string parent_cursor_usr = ngclang::to_string(parent_cursor, &clang_getCursorUSR);

    if (!this->node_exists_by_usr("MemberFunction", cursor_usr))
    {
        {
            mg::Map query_params(2);
            query_params.Insert("display_name", mg::Value(cursor_display_name));
            query_params.Insert("universal_symbol_reference", mg::Value(cursor_usr));

            std::stringstream ss;
            ss << "create(:MemberFunction {"
               << "name: $display_name,"
               << "universal_symbol_reference: $universal_symbol_reference})";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                return false;
            }
        }

        {
            // Class Relationship
            mg::Map query_params(2);
            query_params.Insert("c_usr", mg::Value(parent_cursor_usr));
            query_params.Insert("mf_usr", mg::Value(cursor_usr));

            std::stringstream ss;
            ss << "match (mf:MemberFunction), (c:Class) "
               << "where "
               << "mf.universal_symbol_reference = $mf_usr and "
               << "c.universal_symbol_reference = $c_usr "
               << "create (c)-[:HAS]->(mf)";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                return false;
            }
        }

        {
            // Virtual function overrides
            ngclang::overridden_cursors_t overrides;
            unsigned num_overrides;
            clang_getOverriddenCursors(cursor, &overrides.get(), &num_overrides);

            for(unsigned i = 0; i < num_overrides; ++i)
            {
                CXCursor & override_cursor = overrides.get()[i];
                const std::string override_usr = ngclang::to_string(override_cursor, &clang_getCursorUSR);

                mg::Map query_params(2);
                query_params.Insert("mf_usr", mg::Value(cursor_usr));
                query_params.Insert("override_usr", mg::Value(override_usr));

                std::stringstream ss;
                ss << "match (mf:MemberFunction), (of:MemberFunction) "
                   << "where "
                   << "mf.universal_symbol_reference = $mf_usr and "
                   << "of.universal_symbol_reference = $override_usr "
                   << "create (mf)-[:OVERRIDES]->(of)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }

    const unsigned is_definition = clang_isCursorDefinition(cursor);
    if (is_definition)
    {
        function_decl function_def {cursor};
        function_def_sentry.push(function_def);
        if (!this->node_exists_by_usr("MemberFunctionDefinition", cursor_usr))
        {
            {
                const std::string display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);

                std::stringstream ss;
                ss << "create(:MemberFunctionDefinition {"
                   << "universal_symbol_reference: " << "'" << cursor_usr << "',"
                   << "file: " << "'" << cursor_loc.file() << "',"
                   << "line: " << cursor_loc.line() << ","
                   << "column: " << cursor_loc.column() << ','
                   << "name: " << "'" << display_name << "'"
                   << "})";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str()))
                {
                    return false;
                }
            }

            {
                mg::Map query_params(1);
                query_params.Insert("universal_symbol_reference", mg::Value(function_def.universal_symbol_reference()));

                std::stringstream ss;
                ss << "match(mf:MemberFunction), (mfd:MemberFunctionDefinition) where"
                   << " mf.universal_symbol_reference = $universal_symbol_reference"
                   << " and mfd.universal_symbol_reference = $universal_symbol_reference"
                   << " create (mfd)-[:DEFINES]->(mf)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }
    else
    {
        if (!this->node_exists_by_location("MemberFunctionDeclaration", cursor_loc))
        {
            {
                // Create MemberFunctionDeclaration node
                mg::Map query_params(4);
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value((int) cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));
                query_params.Insert("name", mg::Value(cursor_display_name));

                std::stringstream ss;
                ss << "create(:MemberFunctionDeclaration {"
                   << "name: $name,"
                   << "file: $file,"
                   << "line: $line,"
                   << "column: $column})";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }

            {
                // Create MemberFunctionDeclaration to MemberFunction relationship
                mg::Map query_params(4);
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value((int) cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));
                query_params.Insert("usr", mg::Value(cursor_usr));

                std::stringstream ss;
                ss << "match (mfd:MemberFunctionDeclaration), (mf:MemberFunction) "
                   << "where "
                   << "mfd.file = $file and "
                   << "mfd.line = $line and "
                   << "mfd.column = $column and "
                   << "mf.universal_symbol_reference = $usr "
                   << "create (mfd)-[:DECLARES]->(mf)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool
ast_visitor::graph_constructor(stack_sentry<function_decl> & function_def_sentry,
                               CXCursor cursor,
                               CXCursor parent_cursor)
{
    const cursor_location cursor_loc = cursor_location(cursor);
    const std::string cursor_display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);
    const std::string cursor_usr = ngclang::to_string(cursor, &clang_getCursorUSR);
    const std::string parent_cursor_usr = ngclang::to_string(parent_cursor, &clang_getCursorUSR);

    if (!this->node_exists_by_usr("Constructor", cursor_usr))
    {
        {
            // Create Constructor Node
            mg::Map query_params(2);
            query_params.Insert("display_name", mg::Value(cursor_display_name));
            query_params.Insert("universal_symbol_reference", mg::Value(cursor_usr));

            std::stringstream ss;
            ss << "create(:Constructor {"
               << "name: $display_name,"
               << "universal_symbol_reference: $universal_symbol_reference})";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                return false;
            }
        }

        {
            // Class Relationship
            mg::Map query_params(2);
            query_params.Insert("c_usr", mg::Value(parent_cursor_usr));
            query_params.Insert("ctor_usr", mg::Value(cursor_usr));

            std::stringstream ss;
            ss << "match (ctor:Constructor), (c:Class) "
               << "where "
               << "ctor.universal_symbol_reference = $ctor_usr and "
               << "c.universal_symbol_reference = $c_usr "
               << "create (c)-[:HAS]->(ctor)";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                return false;
            }
        }
    }

    const unsigned is_definition = clang_isCursorDefinition(cursor);
    if (is_definition)
    {
        function_decl function_def {cursor};
        function_def_sentry.push(function_def);
        if (!this->node_exists_by_usr("ConstructorDefinition", cursor_usr))
        {
            {
                const std::string display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);

                std::stringstream ss;
                ss << "create(:ConstructorDefinition {"
                   << "universal_symbol_reference: " << "'" << cursor_usr << "',"
                   << "file: " << "'" << cursor_loc.file() << "',"
                   << "line: " << cursor_loc.line() << ","
                   << "column: " << cursor_loc.column() << ','
                   << "name: " << "'" << display_name << "'"
                   << "})";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str()))
                {
                    return false;
                }
            }

            {
                mg::Map query_params(1);
                query_params.Insert("universal_symbol_reference", mg::Value(function_def.universal_symbol_reference()));

                std::stringstream ss;
                ss << "match(ctor:Constructor), (ctor_d:ConstructorDefinition) where"
                   << " ctor.universal_symbol_reference = $universal_symbol_reference"
                   << " and ctor_d.universal_symbol_reference = $universal_symbol_reference"
                   << " create (ctor_d)-[:DEFINES]->(ctor)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }
    else
    {
        if (!this->node_exists_by_location("ConstructorDeclaration", cursor_loc))
        {
            {
                mg::Map query_params(4);
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value((int) cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));
                query_params.Insert("name", mg::Value(cursor_display_name));

                std::stringstream ss;
                ss << "create(:ConstructorDeclaration {"
                   << "name: $name,"
                   << "file: $file,"
                   << "line: $line,"
                   << "column: $column})";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }

            {
                mg::Map query_params(4);
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value((int) cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));
                query_params.Insert("usr", mg::Value(cursor_usr));

                std::stringstream ss;
                ss << "match (ctor_d:ConstructorDeclaration), (ctor:Constructor) "
                   << "where "
                   << "ctor_d.file = $file and "
                   << "ctor_d.line = $line and "
                   << "ctor_d.column = $column and "
                   << "ctor.universal_symbol_reference = $usr "
                   << "create (ctor_d)-[:DECLARES]->(ctor)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool
ast_visitor::graph_destructor(stack_sentry<function_decl> & function_def_sentry,
                              CXCursor cursor,
                              CXCursor parent_cursor)
{
    const cursor_location cursor_loc = cursor_location(cursor);
    const std::string cursor_display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);
    const std::string cursor_usr = ngclang::to_string(cursor, &clang_getCursorUSR);
    const std::string parent_cursor_usr = ngclang::to_string(parent_cursor, &clang_getCursorUSR);

    if (!this->node_exists_by_usr("Destructor", cursor_usr))
    {
        {
            // Create Destructor Node
            mg::Map query_params(2);
            query_params.Insert("display_name", mg::Value(cursor_display_name));
            query_params.Insert("universal_symbol_reference", mg::Value(cursor_usr));

            std::stringstream ss;
            ss << "create(:Destructor {"
               << "name: $display_name,"
               << "universal_symbol_reference: $universal_symbol_reference})";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                return false;
            }
        }

        {
            // Class Relationship
            mg::Map query_params(2);
            query_params.Insert("c_usr", mg::Value(parent_cursor_usr));
            query_params.Insert("dtor_usr", mg::Value(cursor_usr));

            std::stringstream ss;
            ss << "match (dtor:Destructor), (c:Class) "
               << "where "
               << "dtor.universal_symbol_reference = $dtor_usr and "
               << "c.universal_symbol_reference = $c_usr "
               << "create (c)-[:HAS]->(dtor)";

            ngmg::statement_executor executor(std::ref(*this->_mgclient));
            if (!executor.execute(ss.str(), query_params.AsConstMap()))
            {
                return false;
            }
        }
    }

    const unsigned is_definition = clang_isCursorDefinition(cursor);
    if (is_definition)
    {
        function_decl function_def {cursor};
        function_def_sentry.push(function_def);
        if (!this->node_exists_by_usr("DestructorDefinition", cursor_usr))
        {
            {
                const std::string display_name = ngclang::to_string(cursor, &clang_getCursorDisplayName);

                std::stringstream ss;
                ss << "create(:DestructorDefinition {"
                   << "universal_symbol_reference: " << "'" << cursor_usr << "',"
                   << "file: " << "'" << cursor_loc.file() << "',"
                   << "line: " << cursor_loc.line() << ","
                   << "column: " << cursor_loc.column() << ','
                   << "name: " << "'" << display_name << "'"
                   << "})";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str()))
                {
                    return false;
                }
            }

            {
                mg::Map query_params(1);
                query_params.Insert("universal_symbol_reference", mg::Value(function_def.universal_symbol_reference()));

                std::stringstream ss;
                ss << "match(dtor:Destructor), (dtor_d:DestructorDefinition) where"
                   << " dtor.universal_symbol_reference = $universal_symbol_reference"
                   << " and dtor_d.universal_symbol_reference = $universal_symbol_reference"
                   << " create (dtor_d)-[:DEFINES]->(dtor)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }
    else
    {
        if (!this->node_exists_by_location("DestructorDeclaration", cursor_loc))
        {
            {
                mg::Map query_params(4);
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value((int) cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));
                query_params.Insert("name", mg::Value(cursor_display_name));

                std::stringstream ss;
                ss << "create(:DestructorDeclaration {"
                   << "name: $name,"
                   << "file: $file,"
                   << "line: $line,"
                   << "column: $column})";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }

            {
                mg::Map query_params(4);
                query_params.Insert("file", mg::Value(cursor_loc.file()));
                query_params.Insert("line", mg::Value((int) cursor_loc.line()));
                query_params.Insert("column", mg::Value(cursor_loc.column()));
                query_params.Insert("usr", mg::Value(cursor_usr));

                std::stringstream ss;
                ss << "match (dtor_d:DestructorDeclaration), (dtor:Destructor) "
                   << "where "
                   << "dtor_d.file = $file and "
                   << "dtor_d.line = $line and "
                   << "dtor_d.column = $column and "
                   << "dtor.universal_symbol_reference = $usr "
                   << "create (dtor_d)-[:DECLARES]->(dtor)";

                ngmg::statement_executor executor(std::ref(*this->_mgclient));
                if (!executor.execute(ss.str(), query_params.AsConstMap()))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool
ast_visitor::graph_member_ref_expr(CXCursor cursor, CXCursor parent_cursor)
{
    if (! this->_function_definitions.empty() &&
        ! this->_call_expressions.empty())
    {
        // This is a function call from a function definition
        return this->graph_member_function_call(cursor, parent_cursor);
    }

    return true;
}

bool
ast_visitor::graph_member_function_call(CXCursor cursor, CXCursor parent_cursor)
{
    const cursor_location cursor_loc = cursor_location(cursor);
    CXCursor callee_cursor = clang_getCursorReferenced(cursor);

    const std::string callee_usr = ngclang::to_string(callee_cursor, &clang_getCursorUSR);
    const std::string caller_usr = this->_function_definitions.front().universal_symbol_reference();
    const std::string caller_label = this->_function_definitions.front().is_member_function() ?
        "(caller:MemberFunction)" :
        "(caller:Function)";

    if (this->edge_exists(":CALLS", cursor_loc))
    {
        return true;
    }

    mg::Map query_params(5);
    query_params.Insert("callee_usr", mg::Value(callee_usr));
    query_params.Insert("caller_usr", mg::Value(caller_usr));
    query_params.Insert("file", mg::Value(cursor_loc.file()));
    query_params.Insert("line", mg::Value(cursor_loc.line()));
    query_params.Insert("column", mg::Value(cursor_loc.column()));

    std::stringstream ss;
    ss << "match (callee:MemberFunction),"
       << caller_label
       << " where "
       << " caller.universal_symbol_reference = $caller_usr"
       << " and callee.universal_symbol_reference = $callee_usr"
       << " create (caller)-[:CALLS {file: $file, line: $line, column: $column}]->(callee)";

    ngmg::statement_executor executor(std::ref(*this->_mgclient));
    if (!executor.execute(ss.str(), query_params.AsConstMap()))
    {
        return false;
    }

    return true;
}

std::string
ast_visitor::fully_qualified_namespace() const
{
    std::string name;
    if (this->_namespaces.empty())
    {
        return name;
    }

    name = this->_namespaces.front().name();
    for (auto i = this->_namespaces.cbegin() + 1;
         i != this->_namespaces.cend();
         ++i)
    {
        name += "::";
        name += i->name();
    }

    return name;
}

bool
ast_visitor::edge_exists(const std::string & label,
                         const cursor_location & location)
{
    mg::Map query_params(3);
    query_params.Insert("file", mg::Value(location.file()));
    query_params.Insert("line", mg::Value(location.line()));
    query_params.Insert("column", mg::Value(location.column()));

    std::stringstream ss;
    ss << "match (lhs)-[" << label << " {file: $file, line: $line, column: $column}]->(rhs) return rhs";

    ngmg::statement_executor executor(std::ref(*this->_mgclient));
    if (!executor.execute(ss.str(), query_params.AsConstMap()))
    {
        throw "wtf";
        return false;
    }

    return static_cast<bool>(this->_mgclient->FetchOne());
}

bool
ast_visitor::edge_exists(const std::string & src_label,
                         const ngclang::universal_symbol_reference & src,
                         const std::string & dst_label,
                         const ngclang::universal_symbol_reference & dst,
                         const std::string & edge_label)
{
    // Create namespace to class relationship
    mg::Map query_params(2);
    query_params.Insert("src_usr", mg::Value(src.string()));
    query_params.Insert("dst_usr", mg::Value(dst.string()));

    std::stringstream ss;
    ss << "match (d:"
       << dst_label
       << " {universal_symbol_reference: $dst_usr})<-[:"
       << edge_label << "]-(s:"
       << src_label
       << " {universal_symbol_reference: $src_usr}) return s";

    ngmg::statement_executor executor(std::ref(*this->_mgclient));
    if (!executor.execute(ss.str(), query_params.AsConstMap()))
    {
        throw std::runtime_error("error running: " + ss.str());
    }

    return static_cast<bool>(this->_mgclient->FetchOne());
}

bool
ast_visitor::node_exists_by_location(const std::string & label,
                                     const cursor_location & location)
{
    mg::Map query_params(3);
    query_params.Insert("file", mg::Value(location.file()));
    query_params.Insert("line", mg::Value(location.line()));
    query_params.Insert("column", mg::Value(location.column()));

    std::stringstream ss;
    ss << "match " << '(' << "d:" << label << ')' << " where"
       << " d.file = $file"
       << " and d.line = $line"
       << " and d.column = $column"
       << " return d";

    ngmg::statement_executor executor(std::ref(*this->_mgclient));
    if (!executor.execute(ss.str(), query_params.AsConstMap()))
    {
        return false;
    }

    return static_cast<bool>(this->_mgclient->FetchOne());
}

bool
ast_visitor::node_exists_by_usr(const std::string & label,
                                const std::string & universal_symbol_reference)
{
    mg::Map query_params(1);
    query_params.Insert("universal_symbol_reference", mg::Value(universal_symbol_reference));

    std::stringstream ss;
    ss << "match (fd:" << label << ") where fd.universal_symbol_reference = $universal_symbol_reference return fd";

    ngmg::statement_executor executor(std::ref(*this->_mgclient));
    if (!executor.execute(ss.str(), query_params.AsConstMap()))
    {
        throw std::runtime_error("error running: " + ss.str());
    }

    return static_cast<bool>(this->_mgclient->FetchOne());
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
                           const ast_visitor_filter & filter)
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

    ast_visitor visitor(std::ref(client), std::ref(filter));
    clang_visitChildren(cursor, &ast_visitor::graph, &visitor);
}

int main(int argc, char ** argv)
{
    std::string build_dir;
    std::string file_to_parse;
    bool print_ast = false;

    static const struct option long_options [] = {
        {"src-dir", required_argument, nullptr, 0},
        {"src-tree", required_argument, nullptr, 1},
        {0,0,0,0}
    };

    ast_visitor_filter filter;
    int option_index;
    for(;;)
    {
        switch(::getopt_long(argc, argv, "s:t:d:f:ph",
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
                print_ast = true;
                continue;
            }
            case 'h':
            {
                std::cerr << help_text << std::endl;
                return 0;
            }
            case 's':
            {
                filter.push_back_src_dir(optarg);
                continue;
            }
            case 0:
            {
                filter.push_back_src_dir(optarg);
                continue;
            }
            case 't':
            {
                filter.push_back_src_tree(optarg);
                continue;
            }
            case 1:
            {
                filter.push_back_src_tree(optarg);
                continue;
            }
            case -1:
            {
                break;
            }
        }

        break;
    }
    
    if (print_ast)
    {
        if (file_to_parse.empty())
        {
            std::cerr << "missing file to parse\n";
            return 2;
        }

        CXIndex index = clang_createIndex(0,0);
        CXTranslationUnit unit = clang_parseTranslationUnit(
            index, // clang index
            file_to_parse.c_str(), // path
            nullptr, // command line args
            0, // number of command line args
            nullptr, // clang unsaved files
            0, // number of unsaved files
            CXTranslationUnit_None);

        CXCursor cursor = clang_getTranslationUnitCursor(unit);

        unsigned int level = 1;
        clang_visitChildren(cursor, &printing_visitor, &level);
        return 0;
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

    client->Execute("CREATE INDEX ON :FunctionDefinition(universal_symbol_reference);");
    client->DiscardAll();

    client->Execute("CREATE INDEX ON :MemberFunction(universal_symbol_reference);");
    client->DiscardAll();

    client->Execute("CREATE INDEX ON :MemberFunctionDefinition(universal_symbol_reference);");
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
            if (!filter.parse_file(file_path))
            {
                // The file does not match the specified filter so
                // don't parse.
                continue;
            }

            std::cout << "parsing: " << file_path << std::endl;

            parse_compile_command(index.get(), compile_command, *client, filter);
        }
    }

    return 0;
}
