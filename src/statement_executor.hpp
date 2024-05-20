#ifndef STATEMENT_EXECUTOR_HPP
#define STATEMENT_EXECUTOR_HPP

#include <mgclient.hpp>
#include <memory>
#include <stdexcept>
#include <string>


namespace mg
{
    class Client;
}

namespace ngmg
{
    class execute_error: public std::runtime_error
    {
        public:

        explicit
        execute_error(const std::string & stmt);
    };

    class statement_executor
    {
        public:

        explicit
        statement_executor(std::reference_wrapper<mg::Client> client);

        explicit
        statement_executor(mg::Client &) = delete;

        ~statement_executor() noexcept;

        void
        execute(const std::string & statement);

        void
        execute(const std::string & statement, const mg::ConstMap & params);

        private:

        mg::Client * _client;
        bool _discard_all = false;
    };
}

#endif
