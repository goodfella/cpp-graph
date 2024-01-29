#ifndef STATEMENT_EXECUTOR_HPP
#define STATEMENT_EXECUTOR_HPP

#include <mgclient.hpp>
#include <memory>
#include <string>


namespace mg
{
    class Client;
}

namespace ngmg
{
    class statement_executor
    {
        public:

        explicit
        statement_executor(std::reference_wrapper<mg::Client> client);

        explicit
        statement_executor(mg::Client &) = delete;

        ~statement_executor() noexcept;

        bool
        execute(const std::string & statement);

        bool
        execute(const std::string & statement, const mg::ConstMap & params);

        private:

        mg::Client * _client;
        bool _discard_all = false;
    };
}

#endif
