#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>
#include "ngclang.hpp"
#include <string>

ngclang::universal_symbol_reference::universal_symbol_reference(CXCursor cursor):
    _string(ngclang::to_string(cursor, &clang_getCursorUSR))
{}

const std::string &
ngclang::universal_symbol_reference::string() const noexcept
{
    return this->_string;
}

ngclang::cursor_location::cursor_location(CXCursor cursor)
{
    const CXSourceLocation location = clang_getCursorLocation(cursor);

    CXFile file;
    clang_getExpansionLocation(location,
                               &file,
                               &this->_line,
                               &this->_column,
                               nullptr);

    ngclang::string_t path = clang_File_tryGetRealPathName(file);
    this->_file = ngclang::to_string(path.get());
}

const std::string &
ngclang::cursor_location::file() const noexcept
{
    return this->_file;
}

int
ngclang::cursor_location::line() const noexcept
{
    return this->_line;
}

int
ngclang::cursor_location::column() const noexcept
{
    return this->_column;
}

std::string
ngclang::to_string(CXString cxstring)
{
    char const * const cstr = clang_getCString(cxstring);
    if (!cstr)
    {
        return std::string ();
    }

    return std::string(clang_getCString(cxstring));
}

std::string
ngclang::to_string(CXCursor c, CXString (*f)(CXCursor))
{
    if(clang_Cursor_isNull(c))
    {
        return std::string();
    }

    ngclang::string_t string = f(c);
    return to_string(string.get());
}

std::optional<CXCursor>
ngclang::referenced_cursor(CXCursor cursor) noexcept
{
    CXCursor ref = clang_getCursorReferenced(cursor);
    if (clang_Cursor_isNull(ref))
    {
        return std::optional<CXCursor> {};
    }
    else
    {
        return std::optional<CXCursor> {ref};
    }
}


void
ngclang::dispose_string::operator() (CXString cxstring) const noexcept
{
    clang_disposeString(cxstring);
}

void
ngclang::dispose_compilation_database::operator() (CXCompilationDatabase dbase) const noexcept
{
    clang_CompilationDatabase_dispose(dbase);
}

void
ngclang::dispose_compile_commands::operator()(CXCompileCommands o) const noexcept
{
    clang_CompileCommands_dispose(o);
}

void
ngclang::dispose_index::operator() (CXIndex index) const noexcept
{
    clang_disposeIndex(index);
}


void
ngclang::dispose_translation_unit::operator() (CXTranslationUnit tu) const noexcept
{
    clang_disposeTranslationUnit(tu);
}

void
ngclang::dispose_overridden_cursors::operator() (CXCursor * c) const noexcept
{
    clang_disposeOverriddenCursors(c);
}
