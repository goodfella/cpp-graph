cmake_minimum_required(VERSION 3.22.1)
project(cpp-graph VERSION 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(mgclient)

add_library(ngclang
  src/ngclang.cpp)

target_link_libraries(ngclang
  PRIVATE
  clang-14)

target_compile_options(ngclang
  PRIVATE
  -Wall
  -g3)

target_include_directories(ngclang
  PRIVATE
  /usr/lib/llvm-14/include)

add_executable(cpp-graph
  src/cpp-graph.cpp
  src/statement_executor.cpp)

target_compile_options(cpp-graph
  PRIVATE
  -Wall
  -g3)

set_property(TARGET cpp-graph PROPERTY CXX_STANDARD 20)

target_link_libraries(cpp-graph
  PRIVATE
  clang-14
  mgclient-static
  ngclang
  INTERFACE
  mgclient_cpp)

target_include_directories(cpp-graph
  PRIVATE
  /usr/lib/llvm-14/include
  mgclient/mgclient_cpp/include)
