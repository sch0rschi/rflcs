#include "header/domination_utils.hpp"
#include "../temporaries.hpp"

#include <algorithm>
#include <vector>

bool dominated_by_some_available_but_unused_character(
    const int succ_position_2,
    const int domination_threshold,
    const std::vector<int> &min_positions_2,
    const int min_positions_2_size) {
    return domination_threshold > 0 && min_positions_2_size >= domination_threshold && min_positions_2.at(domination_threshold - 1) < succ_position_2;
}

void add_position_2_to_maybe_min_pos_2(
    std::vector<int> &min_positions_2,
    const int succ_position_2,
    int &min_positions_2_size,
    const int domination_threshold)
{
    if (min_positions_2_size == domination_threshold && succ_position_2 >= min_positions_2[domination_threshold - 1]) {
        return;
    }

    const int insert_pos = std::upper_bound(
        min_positions_2.begin(),
        min_positions_2.begin() + min_positions_2_size,
        succ_position_2
    ) - min_positions_2.begin();

    if (const int move_count = std::min(min_positions_2_size, domination_threshold - 1) - insert_pos + 1; move_count > 0) {
        for (int i = move_count - 1; i >= 0; --i) {
            min_positions_2[insert_pos + i + 1] = min_positions_2[insert_pos + i];
        }
    }

    min_positions_2[insert_pos] = succ_position_2;

    if (min_positions_2_size < domination_threshold) {
        ++min_positions_2_size;
    }
}
