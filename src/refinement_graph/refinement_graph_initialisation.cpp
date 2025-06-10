#include "header/refinement_graph_initialisation.hpp"
#include "../graph/match_loop_utils.h"
#include "header/refinement_graph_filter.hpp"
#include "header/refinement_graph_reduction.hpp"
#include "header/refinement_graph_update.hpp"

void initialize_refinement_graph(instance &instance) {
    absl::flat_hash_map<const rflcs_graph::match *, refinement_node *> refinement_nodes_map;
    for (auto &match: instance.graph->matches
                      | active_match_filter
                      | std::views::reverse
                      | std::views::drop(1)) {
        const auto new_match = new refinement_match();
        new_match->character = match.character;
        new_match->position_1 = match.extension.position_1;
        new_match->position_2 = match.extension.position_2;
        auto new_node = new refinement_node();
        refinement_nodes_map[&match] = new_node;
        new_match->refinement_nodes.push_back(new_node);
        new_node->refinement_match = new_match;

        new_node->characters_on_paths_to_root = match.extension.reversed->extension.available_characters;
        new_node->characters_on_all_paths_to_root = boost::dynamic_bitset<>(instance.alphabet_size);
        new_node->characters_on_paths_to_some_sink = match.extension.available_characters;
        new_node->characters_on_all_paths_to_lower_bound_length = boost::dynamic_bitset<>(instance.alphabet_size);
        if (match.character >= 0 && match.character < instance.alphabet_size) {
            new_node->characters_on_paths_to_root.set(match.character);
            new_node->characters_on_all_paths_to_root.set(match.character);
            new_node->characters_on_paths_to_some_sink.reset(match.character);
            new_node->characters_on_all_paths_to_lower_bound_length.reset(match.character);
            new_node->upper_bound_up = match.extension.reversed->upper_bound;
            new_node->upper_bound_down = 0;
            for (const auto succ_match : match.dom_succ_matches) {
                new_node->upper_bound_down = std::max(new_node->upper_bound_down, succ_match->upper_bound);
            }
        }

        for (const auto *succ: match.extension.succ_matches) {
            if (succ->character >= 0 && succ->character < instance.alphabet_size) {
                const auto succ_refinement_node = refinement_nodes_map[succ];
                new_node->successors.push_back(succ_refinement_node);
                succ_refinement_node->predecessors.push_back(new_node);
            }
        }
        instance.active_refinement_match_pointers.push_back(new_match);
    }
    std::ranges::reverse(instance.active_refinement_match_pointers);
    const auto root_refinement_node = instance.active_refinement_match_pointers.front()->refinement_nodes.front();

    root_refinement_node->upper_bound_up = 0;
    std::ranges::sort(root_refinement_node->successors, [](const refinement_node *refinement_node_1, const refinement_node *refinement_node_2) {
                    return refinement_node_1->refinement_match->position_1 < refinement_node_2->refinement_match->position_1;
                });
    int max_position_2 = INT_MAX;
    for (const auto successor: std::vector(root_refinement_node->successors)) {
        if (successor->refinement_match->position_2 > max_position_2) {
            root_refinement_node->unlink_from_successor(successor);
        } else {
            max_position_2 = std::min(max_position_2, successor->refinement_match->position_2);
        }
    }
    update_refinement_graph(instance);
    write_refinement_graph(instance, "refinement_graph_initial_unfiltered.dot");
    filter_refinement_graph(instance);
    write_refinement_graph(instance, "refinement_graph_initial.dot");
}
