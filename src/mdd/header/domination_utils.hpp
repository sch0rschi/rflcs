#pragma once

#include <vector>

bool dominated_by_some_available_but_unused_character(
    int succ_position_2,
    int domination_threshold,
    const std::vector<int> &min_positions_2,
    int min_positions_2_size);

void add_position_2_to_maybe_min_pos_2(
    std::vector<int> &min_positions_2,
    int succ_position_2,
    int &min_positions_2_size,
    int domination_threshold);
