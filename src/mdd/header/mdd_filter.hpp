#pragma once

#include "../../instance.hpp"

void filter_mdd(instance &instance, mdd &mdd, mdd_node_source &mdd_node_source);

void filter_flat_mdd(const instance &instance, const mdd &mdd, bool is_reporting);
