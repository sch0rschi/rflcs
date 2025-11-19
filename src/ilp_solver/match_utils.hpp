#pragma once

#include "../graph/graph.hpp"
#include "../instance.hpp"

#include <vector>

std::vector<rflcs_graph::match *> get_active_matches(const instance &instance);
