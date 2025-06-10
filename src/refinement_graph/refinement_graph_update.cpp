#include "header/refinement_graph_update.hpp"

#include "header/refinement_graph_dot_writer.hpp"

void update_refinement_graph(const instance &instance) {
    for (const auto refinement_match: instance.active_refinement_match_pointers | std::views::drop(1)) {
        for (const auto refinement_node_to_update: refinement_match->refinement_nodes) {
            int max_upperbound_up = 0;
            globals::old_characters_on_paths_to_root = refinement_node_to_update->characters_on_paths_to_root;
            globals::old_characters_on_all_paths_to_root = refinement_node_to_update->characters_on_all_paths_to_root;
            refinement_node_to_update->characters_on_paths_to_root.reset();
            refinement_node_to_update->characters_on_all_paths_to_root.set();
            for (const auto predecessor_node: refinement_node_to_update->predecessors) {
                refinement_node_to_update->characters_on_paths_to_root |= predecessor_node->characters_on_paths_to_root;
                refinement_node_to_update->characters_on_all_paths_to_root &=
                        predecessor_node->characters_on_all_paths_to_root;
                max_upperbound_up = std::max(max_upperbound_up, predecessor_node->upper_bound_up + 1);
            }
            refinement_node_to_update->characters_on_paths_to_root &= globals::old_characters_on_paths_to_root;
            refinement_node_to_update->characters_on_all_paths_to_root |= globals::old_characters_on_all_paths_to_root;
            refinement_node_to_update->characters_on_paths_to_root.set(refinement_match->character);
            refinement_node_to_update->characters_on_all_paths_to_root.set(refinement_match->character);
            refinement_node_to_update->upper_bound_up = std::min(
                refinement_node_to_update->upper_bound_up, max_upperbound_up);
            refinement_node_to_update->upper_bound_up = std::min(
                refinement_node_to_update->upper_bound_up,
                static_cast<int>(refinement_node_to_update->characters_on_paths_to_root.count()));
        }
    }

    for (const auto refinement_match: instance.active_refinement_match_pointers | std::views::reverse) {
        for (const auto refinement_node_to_update: refinement_match->refinement_nodes) {
            int max_upperbound_down = 0;
            globals::old_characters_on_paths_to_some_sink = refinement_node_to_update->characters_on_paths_to_some_sink;
            globals::old_characters_on_all_paths_to_lower_bound_levels = refinement_node_to_update->characters_on_all_paths_to_lower_bound_length;
            refinement_node_to_update->characters_on_paths_to_some_sink.reset();
            refinement_node_to_update->characters_on_all_paths_to_lower_bound_length.set();
            for (const auto successor_node: refinement_node_to_update->successors) {
                refinement_node_to_update->characters_on_paths_to_some_sink |=
                        successor_node->characters_on_paths_to_some_sink;
                refinement_node_to_update->characters_on_paths_to_some_sink
                        .set(successor_node->refinement_match->character);
                globals::temp_character_set_1 = successor_node->characters_on_all_paths_to_lower_bound_length;
                globals::temp_character_set_1.set(successor_node->refinement_match->character);
                refinement_node_to_update->characters_on_all_paths_to_lower_bound_length &=
                        globals::temp_character_set_1;
                max_upperbound_down = std::max(max_upperbound_down, successor_node->upper_bound_down + 1);
            }
            refinement_node_to_update->upper_bound_down = max_upperbound_down;
            refinement_node_to_update->characters_on_paths_to_some_sink &= globals::old_characters_on_paths_to_some_sink;
            refinement_node_to_update->characters_on_all_paths_to_lower_bound_length |= globals::old_characters_on_all_paths_to_lower_bound_levels;
            if (refinement_node_to_update->upper_bound_up > instance.lower_bound) {
                refinement_node_to_update->characters_on_all_paths_to_lower_bound_length.reset();
            }
            if (refinement_match->character < instance.alphabet_size) {
                refinement_node_to_update->characters_on_paths_to_some_sink.reset(refinement_match->character);
            }
            if (refinement_node_to_update->successors.empty()
                || refinement_node_to_update->upper_bound_up > instance.lower_bound + 1) {
                refinement_node_to_update->characters_on_all_paths_to_lower_bound_length.reset();
            }

            if (refinement_node_to_update->characters_on_paths_to_root.count() == refinement_node_to_update->upper_bound_up) {
                refinement_node_to_update->characters_on_all_paths_to_root = refinement_node_to_update->characters_on_paths_to_root;
            }

            refinement_node_to_update->characters_on_paths_to_some_sink -= refinement_node_to_update->characters_on_all_paths_to_root;
            refinement_node_to_update->characters_on_paths_to_root -= refinement_node_to_update->characters_on_all_paths_to_lower_bound_length;

            refinement_node_to_update->upper_bound_down = std::min(
                refinement_node_to_update->upper_bound_down,max_upperbound_down);
            refinement_node_to_update->upper_bound_down =
                    std::min(refinement_node_to_update->upper_bound_down,
                             static_cast<int>(refinement_node_to_update->characters_on_paths_to_some_sink.count()));
            auto combined_characters = refinement_node_to_update->characters_on_paths_to_root
                                       | refinement_node_to_update->characters_on_paths_to_some_sink;
            refinement_node_to_update->upper_bound = std::min(
                refinement_node_to_update->upper_bound_up + refinement_node_to_update->upper_bound_down,
                static_cast<int>(combined_characters.count()));
        }
    }
}
