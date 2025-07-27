#include "globals.hpp"

int globals::alphabet_size = 0;
std::bitset<CHARACTER_SET_SIZE> globals::temp_character_set_1;
std::bitset<CHARACTER_SET_SIZE> globals::temp_character_set_2;
std::bitset<CHARACTER_SET_SIZE> globals::old_characters_on_paths_to_some_sink;
std::bitset<CHARACTER_SET_SIZE> globals::old_characters_on_all_paths_to_lower_bound_levels;
std::bitset<CHARACTER_SET_SIZE> globals::old_characters_on_paths_to_root;
std::bitset<CHARACTER_SET_SIZE> globals::old_characters_on_all_paths_to_root;
std::vector<int> globals::chaining_numbers;
std::vector<long> globals::node_character_count;
std::vector<long> globals::ingoing_arc_character_count;
std::vector<long> globals::outgoing_arc_character_count;
std::vector<int> globals::int_vector_positions_2;
