#include "statement_executor.hpp"

ngmg::statement_executor::statement_executor(std::reference_wrapper<mg::Client> client):
    _client(&client.get())
{}

bool
ngmg::statement_executor::execute(const std::string & statement)
{
    this->_discard_all = this->_client->Execute(statement);
    return this->_discard_all;
}

bool
ngmg::statement_executor::execute(const std::string & statement, const mg::ConstMap &params)
{
    this->_discard_all = this->_client->Execute(statement, params);
    return this->_discard_all;
}

ngmg::statement_executor::~statement_executor()
{
    if (this->_discard_all)
    {
        this->_client->DiscardAll();
    }
}
