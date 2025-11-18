#pragma once

#include "character_set.hpp"

#include <vector>

struct temporaries {
    static Character_set temp_character_set_1;
    static Character_set temp_character_set_2;
    static Character_set old_characters_on_paths_to_some_sink;
    static Character_set old_characters_on_all_paths_to_lower_bound_levels;
    static Character_set old_characters_on_paths_to_root;
    static Character_set old_characters_on_all_paths_to_root;
    static std::vector<int> chaining_numbers;
    static std::vector<long> node_character_count;
    static std::vector<long> outgoing_edge_character_count;
    static std::vector<long> incoming_edge_character_count;
    static int lower_bound;
    static int upper_bound;
};
