#include "statement_executor.hpp"

ngmg::execute_error::execute_error(const std::string & stmt):
    std::runtime_error("error executing: " + stmt)
{}

ngmg::statement_executor::statement_executor(std::reference_wrapper<mg::Client> client):
    _client(&client.get())
{}

void
ngmg::statement_executor::execute(const std::string & statement)
{
    if (this->_discard_all)
    {
        throw std::logic_error("re-executing statement without prior discard");
    }

    this->_discard_all = this->_client->Execute(statement);
    if (!this->_discard_all)
    {
        throw ngmg::execute_error(statement);
    }
}

void
ngmg::statement_executor::execute(const std::string & statement, const mg::ConstMap &params)
{
    this->_discard_all = this->_client->Execute(statement, params);
    if (!this->_discard_all)
    {
        throw ngmg::execute_error(statement);
    }
}

ngmg::statement_executor::~statement_executor()
{
    if (this->_discard_all)
    {
        this->_client->DiscardAll();
    }
}
