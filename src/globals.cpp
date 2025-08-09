#include "globals.hpp"

int globals::alphabet_size = 0;
Character_set globals::temp_character_set_1;
Character_set globals::temp_character_set_2;
Character_set globals::old_characters_on_paths_to_some_sink;
Character_set globals::old_characters_on_all_paths_to_lower_bound_levels;
Character_set globals::old_characters_on_paths_to_root;
Character_set globals::old_characters_on_all_paths_to_root;
std::vector<int> globals::chaining_numbers;
std::vector<long> globals::node_character_count;
std::vector<long> globals::ingoing_arc_character_count;
std::vector<long> globals::outgoing_arc_character_count;
std::vector<int> globals::int_vector_positions_2;
