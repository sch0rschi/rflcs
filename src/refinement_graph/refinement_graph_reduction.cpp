#include "header/refinement_graph_reduction.hpp"

#include "../config.hpp"
#include "refinement_graph.hpp"
#include "header/refinement_graph_filter.hpp"
#include "header/refinement_graph_refinement.hpp"

std::string bitset_to_index_set(const boost::dynamic_bitset<> &character_set);


void graph_refinement(instance &instance) {
    for (int refinement_round = 0;
         refinement_round < static_cast<character_type>(instance.alphabet_size);
         ++refinement_round) {
        const auto refinement_character = static_cast<character_type>(refinement_round);
        refine_graph_by_character(instance, refinement_character);
        filter_refinement_graph(instance);
        if (IS_WRITING_DOT_FILE) {
            auto filename = std::string("refinement_graph") + std::to_string(refinement_round) + ".dot";
            write_refinement_graph(instance, filename);
        }
        if (instance.active_refinement_match_pointers.front()->refinement_nodes.front()->upper_bound <= instance.lower_bound) {
            return;
        }
    }
}
