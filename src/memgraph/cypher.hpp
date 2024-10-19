#ifndef NGMG_CYPHER_HPP
#define NGMG_CYPHER_HPP

#include "cypher/create_clause.hpp"
#include "cypher/label.hpp"
#include "cypher/match_clause.hpp"
#include "cypher/merge_clause.hpp"
#include "cypher/node_expression.hpp"
#include "cypher/property.hpp"
#include "cypher/property_set.hpp"
#include "cypher/relationship_expression.hpp"
#include "cypher/return_clause.hpp"
#include "cypher/set_clause.hpp"
#include "cypher/variable.hpp"
#include <mgclient.hpp>
#include <sstream>
#include "../statement_executor.hpp"

namespace ngmg::cypher
{
    template <class T>
    struct is_clause:
        std::disjunction<ngmg::cypher::is_create_clause<T>,
                         ngmg::cypher::is_match_clause<T>,
                         ngmg::cypher::is_merge_clause<T>,
                         ngmg::cypher::is_set_clause<T>,
                         ngmg::cypher::is_return_clause<T>> {};

    template <class T>
    concept Clause = ngmg::cypher::is_clause<T>::value;

    namespace detail
    {
        template <ngmg::cypher::Clause T, class ... Args>
        void
        write_clauses(std::ostream & stream, const T & clause, Args && ... args)
        {
            if constexpr (ngmg::cypher::is_create_clause<T>::value)
            {
                static_assert(sizeof...(args) == 0, "create clause cannot be followed by other clauses");
            }

            if constexpr (ngmg::cypher::is_merge_clause<T>::value)
            {
                static_assert(sizeof...(args) == 0, "merge clause cannot be followed by other clauses");
            }

            clause.write(stream);
            if constexpr (sizeof...(args) > 0)
            {
                stream.put(' ');
                ngmg::cypher::detail::write_clauses(stream, std::forward<Args>(args)...);
            }
        }

        template <ngmg::cypher::Property Prop, class ... Props>
        void
        fill_properties(const mg::ConstMap & mg_props, Prop & prop, Props && ... props)
        {
            const auto mg_prop = mg_props.find(prop.name());
            if (mg_prop == mg_props.end())
            {
                throw std::logic_error("missing property");
            }

            if constexpr (std::is_same_v<typename Prop::value_type, int>)
            {
                if ((*mg_prop).second.type() != mg::Value::Type::Int)
                {
                    throw std::logic_error("value type missmatch");
                }

                prop.value((*mg_prop).second.ValueInt());
            }
            else if constexpr (std::is_same_v<typename Prop::value_type, bool>)
            {
                if ((*mg_prop).second.type() != mg::Value::Type::Bool)
                {
                    throw std::logic_error("value type missmatch");
                }

                const bool value = (*mg_prop).second.ValueBool();
                prop.value(value);
            }
            else if constexpr (std::is_same_v<typename Prop::value_type, std::string>)
            {
                if ((*mg_prop).second.type() != mg::Value::Type::String)
                {
                    throw std::logic_error("value type missmatch");
                }

                const std::string value {(*mg_prop).second.ValueString()};
                prop.value(value);
            }

            if constexpr (sizeof...(Props) > 0)
            {
                ngmg::cypher::detail::fill_properties(mg_props, std::forward<Props>(props)...);
            }
        }

        std::optional<mg::Value>
        fetch_relationship(ngmg::statement_executor & executor);
    }

    std::optional<mg::Value>
    fetch_node(ngmg::statement_executor & executor);

    template <ngmg::cypher::PropertyTuple MatchProps>
    std::optional<mg::Value>
    fetch_node(ngmg::statement_executor & executor,
               MatchProps match_props)
    {
        const ngmg::cypher::node_variable match_node_var {"n"};
        const ngmg::cypher::node_expression match_node_expr
            {
                std::cref(match_node_var),
                match_props
            };
        const ngmg::cypher::match_clause match_clause {std::tie(match_node_expr)};
        const ngmg::cypher::return_clause return_clause
            {
                std::cref(match_node_var)
            };

        std::stringstream ss;
        ngmg::cypher::detail::write_clauses(ss, match_clause, return_clause);
        executor.execute(ss.str());

        return ngmg::cypher::fetch_node(executor);
    }

    template <ngmg::cypher::PropertyTuple MatchProps>
    std::optional<mg::Value>
    fetch_node(ngmg::statement_executor & executor,
               const ngmg::cypher::label & label,
               const MatchProps match_props)
    {
        const ngmg::cypher::node_variable match_node_var {"n"};
        const ngmg::cypher::node_expression match_node_expr
            {
                std::cref(match_node_var),
                std::cref(label),
                match_props
            };
        const ngmg::cypher::match_clause match_clause {std::tie(match_node_expr)};
        const ngmg::cypher::return_clause return_clause
            {
                std::cref(match_node_var)
            };

        std::stringstream ss;
        ngmg::cypher::detail::write_clauses(ss, match_clause, return_clause);
        executor.execute(ss.str());

        return ngmg::cypher::fetch_node(executor);
    }

    template <class ... Args>
    void
    write_clauses(std::ostream & stream, Args && ... args)
    {
        ngmg::cypher::detail::write_clauses(stream, std::forward<Args>(args)...);
    }

    template <class ... Args>
    void
    execute(mg::Client & client, Args && ... args)
    {
        std::stringstream ss;
        ngmg::cypher::write_clauses(ss, std::forward<Args>(args)...);

        ngmg::statement_executor executor(std::ref(client));
        executor.execute(ss.str());
    }

    template <ngmg::cypher::PropertyTuple Src,
              ngmg::cypher::PropertyTuple Dst,
              ngmg::cypher::PropertyTuple Props = std::tuple<>>
    void
    create_relate(mg::Client & client,
                  const ngmg::cypher::label & rel_label,
                  const Src & src,
                  const ngmg::cypher::label & src_label,
                  const Dst & dst,
                  const ngmg::cypher::label & dst_label,
                  const ngmg::cypher::relationship_type type = ngmg::cypher::relationship_type::directed,
                  const Props & props = std::tuple<> {})
    {
        const ngmg::cypher::node_variable src_var ("s");
        const ngmg::cypher::node_variable dst_var ("d");

        const ngmg::cypher::node_expression src_node
            {
                std::cref(src_var),
                std::cref(src_label),
                src
            };

        const ngmg::cypher::node_expression dst_node
            {
                std::cref(dst_var),
                std::cref(dst_label),
                dst
            };

        const ngmg::cypher::match_clause match_clause
            {
                std::tie(src_node, dst_node)
            };

        const ngmg::cypher::relationship_expression relationship_expr
            {
                std::cref(src_var),
                std::cref(rel_label),
                std::cref(dst_var),
                type,
                props
            };

        const ngmg::cypher::merge_clause create_clause
            {
                std::tie(relationship_expr)
            };

        ngmg::cypher::execute(client, match_clause, create_clause);
    }

    template <ngmg::cypher::PropertyTuple Src,
              ngmg::cypher::PropertyTuple Dst,
              ngmg::cypher::PropertyTuple Props = std::tuple<>>
    void
    create_relate(mg::Client & client,
                  const ngmg::cypher::label & rel_label,
                  const Src & src,
                  const ngmg::cypher::label & src_label,
                  const Dst & dst,
                  const ngmg::cypher::relationship_type type = ngmg::cypher::relationship_type::directed,
                  const Props & props = std::tuple<> {})
    {
        const ngmg::cypher::node_variable src_var ("s");
        const ngmg::cypher::node_variable dst_var ("d");

        const ngmg::cypher::node_expression src_node
            {
                std::cref(src_var),
                std::cref(src_label),
                src
            };

        const ngmg::cypher::node_expression dst_node
            {
                std::cref(dst_var),
                dst
            };

        const ngmg::cypher::match_clause match_clause
            {
                std::tie(src_node, dst_node)
            };

        const ngmg::cypher::relationship_expression relationship_expr
            {
                std::cref(src_var),
                std::cref(rel_label),
                std::cref(dst_var),
                type,
                props
            };

        const ngmg::cypher::merge_clause create_clause
            {
                std::tie(relationship_expr)
            };

        ngmg::cypher::execute(client, match_clause, create_clause);
    }

    template <ngmg::cypher::PropertyTuple Src,
              ngmg::cypher::PropertyTuple Dst,
              ngmg::cypher::PropertyTuple Props = std::tuple<>>
    void
    create_relate(mg::Client & client,
                  const ngmg::cypher::label & rel_label,
                  const Src & src,
                  const Dst & dst,
                  const ngmg::cypher::relationship_type type = ngmg::cypher::relationship_type::directed,
                  const Props & props = std::tuple<> {})
    {
        const ngmg::cypher::node_variable src_var ("s");
        const ngmg::cypher::node_variable dst_var ("d");

        const ngmg::cypher::node_expression src_node
            {
                std::cref(src_var),
                src
            };

        const ngmg::cypher::node_expression dst_node
            {
                std::cref(dst_var),
                dst
            };

        const ngmg::cypher::match_clause match_clause
            {
                std::tie(src_node, dst_node)
            };

        const ngmg::cypher::relationship_expression relationship_expr
            {
                std::cref(src_var),
                std::cref(rel_label),
                std::cref(dst_var),
                type,
                props
            };

        const ngmg::cypher::merge_clause create_clause
            {
                std::tie(relationship_expr)
            };

        ngmg::cypher::execute(client, match_clause, create_clause);
    }

    template <ngmg::cypher::PropertyTuple Src,
              ngmg::cypher::PropertyTuple Dst,
              ngmg::cypher::PropertyTuple Props = std::tuple<>>
    void
    merge_relate(mg::Client & client,
                 const ngmg::cypher::label & label,
                 const Src & src,
                 const Dst & dst,
                 const ngmg::cypher::relationship_type type = ngmg::cypher::relationship_type::directed,
                 const Props & props = std::tuple<> {})
    {
        const ngmg::cypher::node_variable src_var ("s");
        const ngmg::cypher::node_variable dst_var ("d");

        const ngmg::cypher::node_expression src_node
            {
                std::cref(src_var),
                src
            };

        const ngmg::cypher::node_expression dst_node
            {
                std::cref(dst_var),
                dst
            };

        const ngmg::cypher::match_clause match_clause
            {
                std::tie(src_node, dst_node)
            };

        const ngmg::cypher::relationship_expression relationship_expr
            {
                std::cref(src_var),
                std::cref(label),
                std::cref(dst_var),
                type,
                props
            };

        const ngmg::cypher::merge_clause merge_clause
            {
                std::tie(relationship_expr)
            };

        ngmg::cypher::execute(client, match_clause, merge_clause);
    }

    template <ngmg::cypher::PropertyTuple Src,
              ngmg::cypher::PropertyTuple Dst,
              ngmg::cypher::PropertyTuple Props = std::tuple<>>
    void
    merge_relate(mg::Client & client,
                 const ngmg::cypher::label & label,
                 const Src & src,
                 const ngmg::cypher::label src_label,
                 const Dst & dst,
                 const ngmg::cypher::relationship_type type = ngmg::cypher::relationship_type::directed,
                 const Props & props = std::tuple<> {})
    {
        const ngmg::cypher::node_variable src_var ("s");
        const ngmg::cypher::node_variable dst_var ("d");

        const ngmg::cypher::node_expression src_node
            {
                std::cref(src_var),
                std::cref(src_label),
                src
            };

        const ngmg::cypher::node_expression dst_node
            {
                std::cref(dst_var),
                dst
            };

        const ngmg::cypher::match_clause match_clause
            {
                std::tie(src_node, dst_node)
            };

        const ngmg::cypher::relationship_expression relationship_expr
            {
                std::cref(src_var),
                std::cref(label),
                std::cref(dst_var),
                type,
                props
            };

        const ngmg::cypher::merge_clause merge_clause
            {
                std::tie(relationship_expr)
            };

        ngmg::cypher::execute(client, match_clause, merge_clause);
    }

    template <ngmg::cypher::PropertyTuple MatchProps,
              ngmg::cypher::PropertyTuple SetProps>
    void
    match_set(mg::Client & client,
              const MatchProps & match_props,
              const SetProps & set_props)
    {
        const ngmg::cypher::node_variable match_node_var {"n"};
        const ngmg::cypher::node_expression match_node_expr
            {
                std::cref(match_node_var),
                match_props
            };

        const ngmg::cypher::match_clause match_clause {std::tie(match_node_expr)};
        const ngmg::cypher::set_clause set_clause
            {
                std::cref(match_node_var),
                set_props
            };

        ngmg::cypher::execute(client, match_clause, set_clause);
    }

    template <ngmg::cypher::PropertyTuple MatchProps,
              ngmg::cypher::PropertyTuple ReturnProps>
    bool
    node_return(mg::Client & client,
                const MatchProps & match_props,
                ReturnProps return_props)
    {
        ngmg::statement_executor executor(std::ref(client));
        const std::optional<mg::Value> value = ngmg::cypher::fetch_node(executor, match_props);

        if (!value)
        {
            return false;
        }

        {
            const mg::ConstNode node = value->ValueNode();
            const mg::ConstMap properties = node.properties();
            std::apply([&properties] (auto & ... p) {cypher::detail::fill_properties(properties, p...);}, return_props);
        }

        const auto empty_result = client.FetchOne();
        if (empty_result)
        {
            throw std::logic_error("more than one node matched query");
        }

        return true;
    }

    template <ngmg::cypher::PropertyTuple MatchProps,
              ngmg::cypher::PropertyTuple ReturnProps>
    bool
    node_return(mg::Client & client,
                const ngmg::cypher::label & match_label,
                const MatchProps & match_props,
                ReturnProps return_props)
    {
        ngmg::statement_executor executor(std::ref(client));
        const std::optional<mg::Value> value = ngmg::cypher::fetch_node(executor, match_label, match_props);

        if (!value)
        {
            return false;
        }

        {
            const mg::ConstNode node = value->ValueNode();
            const mg::ConstMap properties = node.properties();
            std::apply([&properties] (auto & ... p) {cypher::detail::fill_properties(properties, p...);}, return_props);
        }

        const auto empty_result = client.FetchOne();
        if (empty_result)
        {
            throw std::logic_error("more than one node matched query");
        }

        return true;
    }

    template <ngmg::cypher::PropertyTuple MatchProps,
              ngmg::cypher::PropertyTuple ReturnProps>
    bool
    node_return(mg::Client & client,
                const ngmg::cypher::label & relationship_label,
                const MatchProps & src_match_props,
                const ngmg::cypher::label & match_label,
                ReturnProps dst_return_props,
                const ngmg::cypher::relationship_type type)
    {
        ngmg::cypher::node_variable src_var {"s"};
        ngmg::cypher::node_expression src_node
            {
                std::cref(src_var),
                src_match_props
            };

        ngmg::cypher::node_variable dst_var {"d"};
        ngmg::cypher::node_expression dst_node
            {
                std::cref(dst_var),
                std::cref(match_label),
                std::tuple<> {}
            };

        ngmg::cypher::return_clause return_clause {std::cref(dst_var)};

        ngmg::cypher::relationship_expression rel
            {
                std::cref(src_node),
                relationship_label,
                std::cref(dst_node),
                type
            };

        ngmg::cypher::match_clause match_clause {std::tie(rel)};

        ngmg::statement_executor executor(std::ref(client));
        std::stringstream ss;
        ngmg::cypher::detail::write_clauses(ss, match_clause, return_clause);
        executor.execute(ss.str());

        std::optional<mg::Value> value = ngmg::cypher::fetch_node(executor);

        if (!value)
        {
            return false;
        }

        {
            const mg::ConstNode node = value->ValueNode();
            const mg::ConstMap properties = node.properties();
            std::apply([&properties] (auto & ... p) {cypher::detail::fill_properties(properties, p...);}, dst_return_props);
        }

        const auto empty_result = client.FetchOne();
        if (empty_result)
        {
            throw std::logic_error("more than one node matched query");
        }

        return true;
    }

    template <ngmg::cypher::PropertyTuple MatchProps>
    bool
    node_exists(mg::Client & client,
                const MatchProps & match_props)
    {
        ngmg::statement_executor executor(std::ref(client));
        const std::optional<mg::Value> value = ngmg::cypher::fetch_node(executor, match_props);

        return value.has_value();
    }

    template <ngmg::cypher::PropertyTuple MatchProps>
    bool
    node_exists(mg::Client & client,
                const ngmg::cypher::label & label,
                const MatchProps & match_props)
    {
        ngmg::statement_executor executor(std::ref(client));
        const std::optional<mg::Value> value = ngmg::cypher::fetch_node(executor, label, match_props);

        return value.has_value();
    }

    template <ngmg::cypher::PropertyTuple SrcProps,
              ngmg::cypher::PropertyTuple DstProps,
              ngmg::cypher::PropertyTuple EdgeProps = std::tuple<>>
    bool
    relationship_exists(mg::Client & client,
                        const ngmg::cypher::label & edge_label,
                        const SrcProps & src_props,
                        const DstProps & dst_props,
                        const ngmg::cypher::relationship_type type = ngmg::cypher::relationship_type::directed,
                        const EdgeProps & edge_props = std::tuple<> {})
    {
        const ngmg::cypher::node_variable src_var {"s"};
        const ngmg::cypher::node_expression src_node_expr
            {
                std::cref(src_var),
                src_props
            };

        const ngmg::cypher::node_variable dst_var {"d"};
        const ngmg::cypher::node_expression dst_node_expr
            {
                std::cref(dst_var),
                dst_props
            };

        const ngmg::cypher::relationship_variable rel_var {"r"};
        const ngmg::cypher::relationship_expression rel_expr
            {
                std::cref(rel_var),
                std::cref(src_node_expr),
                std::cref(edge_label),
                std::cref(dst_node_expr),
                type,
                edge_props
            };

        const ngmg::cypher::match_clause match_clause
            {
                std::tie(rel_expr)
            };

        const ngmg::cypher::return_clause return_clause {std::cref(rel_var)};

        ngmg::statement_executor executor(std::ref(client));
        std::stringstream ss;

        ngmg::cypher::detail::write_clauses(ss, match_clause, return_clause);

        executor.execute(ss.str());
        const std::optional<mg::Value> relationship =
            ngmg::cypher::detail::fetch_relationship(executor);

        return relationship.has_value();
    }

    template <ngmg::cypher::PropertyTuple EdgeProps = std::tuple<>>
    bool
    relationship_exists(mg::Client & client,
                        const ngmg::cypher::label & edge_label,
                        const ngmg::cypher::relationship_type type = ngmg::cypher::relationship_type::directed,
                        const EdgeProps & edge_props = std::tuple<> {})
    {
        const ngmg::cypher::node_variable src_var {"s"};
        const ngmg::cypher::node_variable dst_var {"d"};

        const ngmg::cypher::relationship_variable rel_var {"r"};
        const ngmg::cypher::relationship_expression rel_expr
            {
                std::cref(rel_var),
                std::cref(src_var),
                std::cref(edge_label),
                std::cref(dst_var),
                type,
                edge_props
            };

        const ngmg::cypher::match_clause match_clause
            {
                std::tie(rel_expr)
            };

        const ngmg::cypher::return_clause return_clause {std::cref(rel_var)};

        ngmg::statement_executor executor(std::ref(client));
        std::stringstream ss;

        ngmg::cypher::detail::write_clauses(ss, match_clause, return_clause);

        executor.execute(ss.str());
        const std::optional<mg::Value> relationship =
            ngmg::cypher::detail::fetch_relationship(executor);

        return relationship.has_value();
    }

    template <ngmg::cypher::PropertyTuple SrcProps,
        ngmg::cypher::PropertyTuple DstProps,
        ngmg::cypher::PropertyTuple EdgeProps = std::tuple<>>
    bool
    relationship_exists(mg::Client & client,
                        const ngmg::cypher::label & edge_label,
                        const SrcProps & src_props,
                        const ngmg::cypher::label & src_label,
                        const DstProps & dst_props,
                        const ngmg::cypher::label & dst_label,
                        const ngmg::cypher::relationship_type type = ngmg::cypher::relationship_type::directed,
                        const EdgeProps & edge_props = std::tuple<> {})
    {
        const ngmg::cypher::node_variable src_var {"s"};
        const ngmg::cypher::node_expression src_node_expr
            {
                std::cref(src_var),
                std::cref(src_label),
                src_props
            };

        const ngmg::cypher::node_variable dst_var {"d"};
        const ngmg::cypher::node_expression dst_node_expr
            {
                std::cref(dst_var),
                std::cref(dst_label),
                dst_props
            };

        const ngmg::cypher::relationship_variable rel_var {"r"};
        const ngmg::cypher::relationship_expression rel_expr
            {
                std::cref(rel_var),
                std::cref(src_node_expr),
                std::cref(edge_label),
                std::cref(dst_node_expr),
                type,
                edge_props
            };

        const ngmg::cypher::match_clause match_clause
            {
                std::tie(rel_expr)
            };

        const ngmg::cypher::return_clause return_clause {std::cref(rel_var)};

        ngmg::statement_executor executor(std::ref(client));
        std::stringstream ss;

        ngmg::cypher::detail::write_clauses(ss, match_clause, return_clause);

        executor.execute(ss.str());
        const std::optional<mg::Value> relationship =
            ngmg::cypher::detail::fetch_relationship(executor);

        return relationship.has_value();
    }

    template <ngmg::cypher::PropertyTuple Props>
    void
    create_node(mg::Client & client,
                const std::set<ngmg::cypher::label> & label_set,
                const Props & props,
                const ngmg::cypher::property_set & property_set)
    {
        const ngmg::cypher::node_expression create_node_expr
            {
                std::cref(label_set),
                props,
                std::cref(property_set)
            };
        const ngmg::cypher::create_clause node_create {std::tie(create_node_expr)};

        ngmg::cypher::execute(client, node_create);
    }

    template <ngmg::cypher::PropertyTuple Props>
    void
    create_node(mg::Client & client,
                const std::set<ngmg::cypher::label> & label_set,
                const Props & props)
    {
        const ngmg::cypher::node_expression create_node_expr
            {
                std::cref(label_set),
                props
            };

        const ngmg::cypher::create_clause node_create {std::tie(create_node_expr)};

        ngmg::cypher::execute(client, node_create);
    }

    template <ngmg::cypher::PropertyTuple Props>
    void
    create_node(mg::Client & client,
                const ngmg::cypher::label & label,
                const Props & props)
    {
        const ngmg::cypher::node_expression create_node_expr
            {
                std::cref(label),
                props
            };
        const ngmg::cypher::create_clause node_create {std::tie(create_node_expr)};

        ngmg::cypher::execute(client, node_create);
    }

    template <ngmg::cypher::PropertyTuple Props>
    void
    create_node(mg::Client & client,
                const ngmg::cypher::label & label,
                const Props & props,
                const ngmg::cypher::property_set & prop_set)
    {
        const ngmg::cypher::node_expression create_node_expr
            {
                std::cref(label),
                props,
                std::cref(prop_set)
            };
        const ngmg::cypher::create_clause node_create {std::tie(create_node_expr)};

        ngmg::cypher::execute(client, node_create);
    }
}

#endif
