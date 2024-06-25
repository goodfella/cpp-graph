#ifndef NGMG_CYPHER_HPP
#define NGMG_CYPHER_HPP

#include "cypher/create_clause.hpp"
#include "cypher/label.hpp"
#include "cypher/label_set.hpp"
#include "cypher/node_expression.hpp"
#include "cypher/property.hpp"
#include "cypher/property_factory.hpp"
#include "cypher/property_set.hpp"
#include "cypher/relationship_expression.hpp"
#include "cypher/variable.hpp"
#include <mgclient.hpp>
#include <sstream>
#include "../statement_executor.hpp"

namespace ngmg::cypher
{
    template <class Create_Clause>
    requires ngmg::cypher::is_create_clause<Create_Clause>::value
    void
    execute(mg::Client & client, const Create_Clause & create_clause)
    {
        std::stringstream ss;
        create_clause.write(ss);

        ngmg::statement_executor executor(std::ref(client));
        executor.execute(ss.str());
    }
}

#endif
