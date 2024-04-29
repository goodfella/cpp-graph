#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>
#include <string>

namespace ngclang
{
    std::string
    to_string(CXString cxstring);

    std::string
    to_string(CXCursor c, CXString (*f)(CXCursor));

    template<class T, class D>
    class object
    {
        public:

        object(T object) noexcept;
        object() noexcept = default;

        ~object() noexcept;

        object(const object &) = delete;
        object(object &&) = delete;

        object & operator() (const object &) = delete;
        object & operator() (const object &&) = delete;

        T & get() noexcept;
        const T & get() const noexcept;

        private:

        T _object;
    };

    template <class T, class D>
    object<T,D>::object(T object) noexcept:
        _object(object)
    {}

    template<class T, class D>
    object<T,D>::~object() noexcept
    {
        D deleter;
        deleter(this->_object);
    }

    template<class T, class D>
    T &
    object<T,D>::get() noexcept
    {
        return this->_object;
    }

    template<class T, class D>
    const T &
    object<T,D>::get() const noexcept
    {
        return this->_object;
    }


    class universal_symbol_reference
    {
        public:

        explicit
        universal_symbol_reference(CXCursor cursor);

        const std::string &
        string() const noexcept;

        private:

        std::string _string;
    };


    struct dispose_string
    {
        void
        operator () (CXString) const noexcept;
    };

    struct dispose_compilation_database
    {
        void
        operator () (CXCompilationDatabase) const noexcept;
    };

    struct dispose_compile_commands
    {
        void
        operator () (CXCompileCommands) const noexcept;
    };

    struct dispose_index
    {
        void
        operator () (CXIndex) const noexcept;
    };

    struct dispose_translation_unit
    {
        void
        operator () (CXTranslationUnit) const noexcept;
    };

    struct dispose_overridden_cursors
    {
        void
        operator () (CXCursor * overridden) const noexcept;
    };

    using translation_unit_t = ngclang::object<CXTranslationUnit, ngclang::dispose_translation_unit>;
    using string_t = ngclang::object<CXString, ngclang::dispose_string>;
    using overridden_cursors_t = ngclang::object<CXCursor*, ngclang::dispose_overridden_cursors>;
}
