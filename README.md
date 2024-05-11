# Introduction

cpp-graph is a program that parses C++ code using
[libclang](https://clang.llvm.org/doxygen/group__CINDEX.html) to
create a graph representation of the parsed code with
[memgraph](https://memgraph.com/).

The memgraph graph schema is shown below:

![Graph Schema](./doc/assets/graph-schema.png)

# Building

Building cpp-graph is best done with the
[cpp-graph-project](https://github.com/goodfella/cpp-graph-project)
repository since it brings in the necessary dependencies and contains
a top level CMakeLists.txt that contains the required additional build
directives.

# Examples
## Call graph:
This cypher query returns functions and member functions and the
functions and/or member functions they call.

```Cypher
match (a)-[r:CALLS]->(b)
return a,r,b
```
![Call Graph](./doc/assets/call-graph.png)

## Code organization
This cypher query returns the following:
- Namespaces and the classes and functions they contain
- Classes and the classes and member functions they contain

```Cypher
match (a)-[r:HAS]->(b)
return a,r,b
```
![Code Organization](./doc/assets/code-organization.png)
