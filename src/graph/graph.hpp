#pragma once

#include <climits>
#include <vector>

#include "../character_set.hpp"
#include "../character.hpp"

#define MATCH_BINDINGS(P) \
P##character, P##upper_bound, P##dom_succ_matches, \
P##heuristic_characters, P##heuristic_successor_match, \
P##reversed, P##extension, P##is_active

namespace rflcs_graph {

    struct match;

    struct match_extension {
        int match_id;
        int combined_upper_bound = std::numeric_limits<int>::max();
        std::vector<match *> succ_matches = std::vector<match *>();
        std::vector<match *> pred_matches = std::vector<match *>();
        std::vector<match *> dom_pred_matches = std::vector<match *>(); // incoming dominating edges from predecessors
        std::vector<int> rf_relaxed_upper_bounds = std::vector<int>();
        Character_set available_characters;
        std::vector<int> repetition_counter;
        int position_1;
        int position_2;
        int transient_match_domination_number;
        int lcs_depth = 0;
    };

    struct match {
        Character character = MAX_CHARACTER;
        int upper_bound = std::numeric_limits<int>::max(); // including this character
        std::vector<match *> dom_succ_matches = std::vector<match *>();
        Character_set heuristic_characters;
        match* heuristic_successor_match;
        match* reversed;
        match_extension* extension;
        bool is_active = true;
    };

    struct graph {
        std::vector<match> matches = std::vector<match>();
        std::vector<match> reverse_matches = std::vector<match>();
    };

    inline auto position_1_comparator(const match* first, const match* second) -> bool {
        return first->extension->position_1 < second->extension->position_1;
    }
}
