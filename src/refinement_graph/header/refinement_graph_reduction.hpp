#ifndef REFINEMENT_GRAPH_REDUCTION_HPP
#define REFINEMENT_GRAPH_REDUCTION_HPP

#include "../../instance.hpp"
#include "../../mdd/mdd.hpp"

void graph_refinement(const instance & instance);

void write_refinement_graph(const instance & instance, const std::string &filename);

#endif
