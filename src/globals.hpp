#pragma once

#include <vector>

#include "character_set.hpp"

struct globals {
    static int alphabet_size;
    static Character_set temp_character_set_1;
    static Character_set temp_character_set_2;
    static Character_set old_characters_on_paths_to_some_sink;
    static Character_set old_characters_on_all_paths_to_lower_bound_levels;
    static Character_set old_characters_on_paths_to_root;
    static Character_set old_characters_on_all_paths_to_root;
    static std::vector<int> chaining_numbers;
    static std::vector<long> node_character_count;
    static std::vector<long> outgoing_arc_character_count;
    static std::vector<long> ingoing_arc_character_count;
    static std::vector<int> int_vector_positions_2;
};
