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
