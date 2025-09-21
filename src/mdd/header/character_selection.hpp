#pragma once

#include <boost/timer/progress_display.hpp>

#include "../../instance.hpp"

void update_characters_ordered_by_importance_mdd(std::vector<Character> &characters_ordered_by_importance,
                                                 const instance &instance,
                                                 const mdd &reduction_mdd,
                                                 mdd_node_source &mdd_node_source,
                                                 boost::timer::progress_display *progress);
