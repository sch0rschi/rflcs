#pragma once

#include "../graph.hpp"

void write_graph_dot(const std::vector<rflcs_graph::match> &matches,
                     const std::vector<Character> &string1,
                     const std::vector<Character> &string2,
                     const char* initial_dot_filename,
                     bool is_reverse);
