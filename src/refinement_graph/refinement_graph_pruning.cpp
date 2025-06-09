#include "header/refinement_graph_pruning.hpp"

void prune_refinement_graph(const instance &instance) {
    for (const auto match: instance.active_refinement_match_pointers | std::views::drop(1)) {
        for (auto *node: std::vector(match->refinement_nodes)) {
            if (node->predecessors.empty() || node->upper_bound <= instance.lower_bound) {
                node->prune();
            } else {
                std::ranges::sort(node->successors,
                                  [](const refinement_node *refinement_node_1,
                                     const refinement_node *refinement_node_2) {
                                      return refinement_node_1->refinement_match->position_1 < refinement_node_2->
                                             refinement_match->position_1;
                                  });
                int max_position_2 = INT_MAX;
                for (const auto successor: std::vector(node->successors)) {
                    const int combined_upper_bound = node->upper_bound_up + successor->upper_bound_down + 1;
                    globals::temp_character_set_1 = node->characters_on_paths_to_root;
                    globals::temp_character_set_1 |= successor->characters_on_paths_to_some_sink;
                    globals::temp_character_set_1.set(successor->refinement_match->character);
                    if (!node->characters_on_paths_to_some_sink.test(successor->refinement_match->character)
                        || combined_upper_bound <= instance.lower_bound
                        || globals::temp_character_set_1.count() <= instance.lower_bound
                        || successor->refinement_match->position_2 > max_position_2) {
                        node->unlink_from_successor(successor);
                    } else {
                        if (!node->characters_on_paths_to_root.test(successor->refinement_match->character)) {
                            max_position_2 = std::min(max_position_2, successor->refinement_match->position_2);
                        }
                    }
                }
            }
        }
    }
}
