#include "header/refinement_graph_filter.hpp"

#include "header/refinement_graph_pruning.hpp"
#include "header/refinement_graph_update.hpp"


void filter_refinement_graph(const instance &instance) {
    update_refinement_graph(instance);
    prune_refinement_graph(instance);
}
