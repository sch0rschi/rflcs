#pragma once

#include "../../instance.hpp"

using comparison_tuple = std::tuple<int, int, int, int, int>;

std::vector<int> get_single_character_repetitions(rflcs_graph::graph &graph);

std::vector<int> get_characters_ordered_by_importance(rflcs_graph::graph &graph);
