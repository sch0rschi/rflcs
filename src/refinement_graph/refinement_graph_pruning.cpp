#include "header/refinement_graph_pruning.hpp"

#include "boost/exception/detail/clone_current_exception.hpp"

void prune_refinement_graph(const instance &instance) {
    for (const auto match: instance.active_refinement_match_pointers | std::views::drop(1)) {
        for (auto *node: std::vector(match->refinement_nodes)) {
            globals::temp_character_set_1 = node->characters_on_all_paths_to_root;
            globals::temp_character_set_1 &= node->characters_on_all_paths_to_lower_bound_length;
            globals::temp_character_set_2 = node->characters_on_paths_to_root;
            globals::temp_character_set_2 |= node->characters_on_paths_to_some_sink;
            if (node->predecessors.empty()
                || node->upper_bound <= instance.lower_bound
                || globals::temp_character_set_1.any()
                || globals::temp_character_set_2.count() <= instance.lower_bound
                || node->characters_on_all_paths_to_root.count() > node->upper_bound_up
            ) {
                node->prune();
            } else {
                std::ranges::sort(node->successors,
                                  [](const refinement_node *refinement_node_1,
                                     const refinement_node *refinement_node_2) {
                                      return refinement_node_1->refinement_match->position_1 < refinement_node_2->
                                             refinement_match->position_1;
                                  });
                auto positions_2 = std::vector<int>();
                const int max_dominating_count =
                        node->upper_bound_up - node->characters_on_all_paths_to_root.count() + 1;
                int max_position_2 = INT_MAX;
                for (const auto successor: std::vector(node->successors)) {
                    const int combined_upper_bound = node->upper_bound_up + successor->upper_bound_down + 1;
                    globals::temp_character_set_1 = node->characters_on_paths_to_root;
                    globals::temp_character_set_1 |= successor->characters_on_paths_to_some_sink;
                    globals::temp_character_set_1.set(successor->refinement_match->character);
                    const bool combined_characters_not_sufficient =
                            globals::temp_character_set_1.count() <= instance.lower_bound;
                    globals::temp_character_set_2 = node->characters_on_all_paths_to_root;
                    globals::temp_character_set_2 &= successor->characters_on_all_paths_to_lower_bound_length;
                    const bool repetition_free_conflict = globals::temp_character_set_2.any();
                    globals::temp_character_set_1 = node->characters_on_paths_to_some_sink;
                    globals::temp_character_set_1.flip();
                    globals::temp_character_set_1 &= successor->characters_on_all_paths_to_lower_bound_length;
                    const bool ruled_out_character_appears = globals::temp_character_set_1.any();
                    const bool suboptimal =
                            successor->characters_on_paths_to_root == successor->characters_on_all_paths_to_root
                            && successor->upper_bound_up - 1 > node->upper_bound_up;
                    if (!node->characters_on_paths_to_some_sink.test(successor->refinement_match->character)
                        || combined_upper_bound <= instance.lower_bound
                        || combined_characters_not_sufficient
                        || repetition_free_conflict
                        || successor->refinement_match->position_2 > max_position_2
                        || ruled_out_character_appears
                        || (positions_2.size() >= max_dominating_count
                            && successor->refinement_match->position_2 > positions_2.at(max_dominating_count - 1))
                        || suboptimal
                    ) {
                        node->unlink_from_successor(successor);
                    } else {
                        if (!node->characters_on_paths_to_root.test(successor->refinement_match->character)) {
                            max_position_2 = std::min(max_position_2, successor->refinement_match->position_2);
                        }
                        positions_2.push_back(successor->refinement_match->position_2);
                        std::ranges::sort(positions_2);
                    }
                }
            }
        }
    }
}
