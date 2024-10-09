cmake_minimum_required(VERSION 3.22.1)
project(cpp-graph VERSION 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(ngclang
  src/ngclang.cpp)

set_property(TARGET ngclang PROPERTY CXX_STANDARD 23)

target_link_libraries(ngclang
  PRIVATE
  clang-18)

target_compile_options(ngclang
  PRIVATE
  -Wall
  -g3)

target_include_directories(ngclang
  PRIVATE
  /usr/lib/llvm-18/include)

file(READ src/help.txt HELP_TEXT)
configure_file(src/help.cpp.in src/generated/help.cpp @ONLY)

add_executable(cpp-graph
  src/cpp-graph.cpp
  src/statement_executor.cpp
  src/generated/help.cpp
  src/edge_labels.cpp
  src/node_labels.cpp
  src/raw_node.cpp
  src/has_reference_property.cpp
  src/location_properties.cpp
  src/name_properties.cpp
  src/is_template_property.cpp
  src/universal_symbol_reference_property.cpp
  src/namespace_node.cpp
  src/namespace_decl_node.cpp
  src/call_expr_node.cpp
  src/class_decl_node.cpp
  src/class_node.cpp
  src/function_node.cpp
  src/function_decl_def_node.cpp
  src/unknown_callee_node.cpp
  src/memgraph/cypher.cpp
  src/memgraph/cypher/property.cpp
  src/memgraph/cypher/property_set.cpp
  src/memgraph/cypher/label.cpp
  src/memgraph/cypher/variable.cpp
  src/memgraph/cypher/node_expression.cpp
  src/memgraph/cypher/relationship_expression.cpp
  src/memgraph/cypher/return_clause.cpp
  src/memgraph/cypher/set_clause.cpp)

target_compile_options(cpp-graph
  PRIVATE
  -Wall
  -g3)

set_property(TARGET cpp-graph PROPERTY CXX_STANDARD 23)

target_link_libraries(cpp-graph
  PRIVATE
  mgclient
  ngclang)
