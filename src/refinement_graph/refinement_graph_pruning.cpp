#include "header/refinement_graph_pruning.hpp"

void prune_refinement_graph(const instance &instance) {
    for (const auto match: instance.active_refinement_match_pointers | std::views::drop(1)) {
        for (auto *node: std::vector(match->refinement_nodes)) {
            if (node->predecessors.empty()
                || node->upper_bound <= instance.lower_bound
                ) {
                node->prune();
            }
            for (const auto successor: std::vector(node->successors)) {
                if (!node->characters_on_paths_to_some_sink.test(successor->refinement_match->character)) {
                    node->unlink_from_succ(successor);
                }
            }
        }
    }
}
