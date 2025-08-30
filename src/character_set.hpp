#pragma once

#ifdef CHARACTER_SET_SIZE
#include <bitset>

struct Character_set : std::bitset<CHARACTER_SET_SIZE> {
    Character_set() : std::bitset<CHARACTER_SET_SIZE>() {}

    Character_set& operator-=(const Character_set& other) {
        static std::bitset<CHARACTER_SET_SIZE> temp;
        temp = ~other;
        *this &= temp;
        return *this;
    }
};

#else
#include "globals.hpp"
#include "boost/dynamic_bitset/dynamic_bitset.hpp"

struct Character_set : boost::dynamic_bitset<> {
    Character_set() : dynamic_bitset(globals::alphabet_size) {}
};

#endif
