#pragma once

#ifdef CHARACTER_SET_SIZE
#include <bitset>

struct Character_set : std::bitset<CHARACTER_SET_SIZE> {
    Character_set() = default;
};

#else
#include "constants.hpp"
#include <boost/dynamic_bitset.hpp>

#include "character.hpp"

struct Character_set : boost::dynamic_bitset<> {
    Character_set() : dynamic_bitset(constants::alphabet_size) {}
};

#endif
