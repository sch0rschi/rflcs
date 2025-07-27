#ifndef CHARACTER_BOUND_UTILS_HPP
#define CHARACTER_BOUND_UTILS_HPP

#include <bitset>
#include "../../config.hpp"

bool are_enough_characters_available(int lower_bound,
                                     int depth,
                                     const std::bitset<CHARACTER_SET_SIZE> &pred_available_characters,
                                     const std::bitset<CHARACTER_SET_SIZE> &succ_available_characters);

#endif