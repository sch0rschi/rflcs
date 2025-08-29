#pragma once

#ifdef CHARACTER_SET_SIZE
    #include <bitset>
    typedef std::bitset<CHARACTER_SET_SIZE> Character_set;
    #define MAKE_CHARACTER_SET() Character_set()
#else
    #include "boost/dynamic_bitset/dynamic_bitset.hpp"
    typedef boost::dynamic_bitset<> Character_set;
    #define MAKE_CHARACTER_SET() Character_set(globals::alphabet_size)
#endif
