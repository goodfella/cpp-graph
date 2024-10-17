#ifndef EDGE_LABELS_HPP
#define EDGE_LABELS_HPP

#include "memgraph/cypher/label.hpp"

extern const ngmg::cypher::label has_label;
extern const ngmg::cypher::label declares_label;
extern const ngmg::cypher::label inherits_label;
extern const ngmg::cypher::label calls_label;
extern const ngmg::cypher::label defines_label;
extern const ngmg::cypher::label overrides_label;
extern const ngmg::cypher::label targets_label;
extern const ngmg::cypher::label references_label;
extern const ngmg::cypher::label lexical_parent_label;
extern const ngmg::cypher::label semantic_parent_label;

#endif
