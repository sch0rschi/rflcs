#include "header/refinement_graph_refinement.hpp"

void refine_graph_by_character(instance &instance, const character_type refinement_character) {
    std::cout << "refining character: " << refinement_character << std::endl;
    for (const auto refinement_match: instance.active_refinement_match_pointers) {
        for (const auto split_node_fixed_previous: std::vector(refinement_match->refinement_nodes)) {
            if (split_node_fixed_previous->characters_on_paths_to_root.test(refinement_character)
                && split_node_fixed_previous->characters_on_paths_to_some_sink.test(refinement_character)) {
                split_node_fixed_previous->split(refinement_character);
                instance.refinement_graph_refinement_count++;
            }
        }
    }
}
