#include "match_utils.hpp"
#include "../graph/match_loop_utils.hpp"

std::vector<rflcs_graph::match *> get_active_matches(const instance &instance) {
    const auto matches = instance.is_solving_forward ? &instance.graph->matches : &instance.graph->reverse_matches;
    auto active_matches = std::vector<rflcs_graph::match *>();
    for (auto &match: *matches | active_match_filter) {
        active_matches.push_back(&match);
    }
    return active_matches;
}
