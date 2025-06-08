#include "header/refinement_graph_reduction.hpp"

#include "header/graphiz_utils.hpp"
#include <fstream>

#include "../graph/match_loop_utils.h"
#include "../config.hpp"
#include "refinement_graph.hpp"
#include "header/refinement_graph_filter.hpp"
#include "header/refinement_graph_refinement.hpp"

std::string bitset_to_index_set(const boost::dynamic_bitset<> &character_set);

void graph_refinement(const instance &instance) {
    for (character_type refinement_character = 0;
         refinement_character < static_cast<character_type>(instance.alphabet_size); ++refinement_character) {
        refine_graph_by_character(instance, refinement_character);
        filter_refinement_graph(instance);
        if (IS_WRITING_DOT_FILE) {
            auto filename = std::string("refinement_graph") + std::to_string(refinement_character) + ".dot";
            write_refinement_graph(instance, filename);
        }
    }
}
