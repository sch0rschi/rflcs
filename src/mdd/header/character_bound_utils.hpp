#pragma once

#include "../../character_set.hpp"

bool are_enough_characters_available(int lower_bound,
                                     int depth,
                                     const Character_set &pred_available_characters,
                                     const Character_set &succ_available_characters);
