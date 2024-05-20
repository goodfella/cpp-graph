cmake_minimum_required(VERSION 3.22.1)
project(cpp-graph VERSION 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

file(READ src/help.txt HELP_TEXT)
configure_file(src/help.cpp.in src/generated/help.cpp @ONLY)

add_executable(cpp-graph
  src/cpp-graph.cpp
  src/statement_executor.cpp
  src/generated/help.cpp)

target_compile_options(cpp-graph
  PRIVATE
  -Wall
  -g3)

set_property(TARGET cpp-graph PROPERTY CXX_STANDARD 23)

target_link_libraries(cpp-graph
  PRIVATE
  clang-14
  mgclient
  ngclang)
