#include "cypher.hpp"
#include <mgclient.hpp>
#include <optional>
#include <type_traits>

std::optional<mg::Value>
ngmg::cypher::detail::fetch_node(ngmg::statement_executor & executor)
{
    const std::optional<std::vector<mg::Value>> maybe_result = executor.client().FetchOne();
    if (!maybe_result || maybe_result->size() < 1)
    {
        return std::optional<mg::Value> {};
    }

    const std::vector<mg::Value> & result = *maybe_result;
    const mg::Value & value = result[0];
    if (value.type() != mg::Value::Type::Node)
    {
        throw std::logic_error("invalid type match");
    }

    return std::optional<mg::Value> {std::move(value)};
}

std::optional<mg::Value>
ngmg::cypher::detail::fetch_relationship(ngmg::statement_executor & executor)
{
    const std::optional<std::vector<mg::Value>> maybe_result = executor.client().FetchOne();
    if (!maybe_result || maybe_result->size() < 1)
    {
        return std::optional<mg::Value> {};
    }

    const std::vector<mg::Value> & result = *maybe_result;
    const mg::Value & value = result[0];
    if (value.type() != mg::Value::Type::Relationship)
    {
        throw std::logic_error("invalid type match");
    }

    return std::optional<mg::Value> {std::move(value)};
}
