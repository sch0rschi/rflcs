#pragma once

#ifndef CHARACTER_SET_SIZE
#define CHARACTER_SET_SIZE 1024
#endif
#define DYNAMIC_SET_SIZE_FEATURE

#ifdef DYNAMIC_SET_SIZE_FEATURE
    #include "boost/dynamic_bitset/dynamic_bitset.hpp"

    typedef boost::dynamic_bitset<> Character_set;

    inline Character_set make_character_set(const int size) {
        return Character_set(size);
    }
#else
    #include <bitset>

    typedef std::bitset<CHARACTER_SET_SIZE> Character_set;

    inline Character_set make_character_set(const int size) {
        return {};
    }
#endif