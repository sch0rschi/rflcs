#include "mdd_graph_pruning.hpp"
#include "absl/container/flat_hash_set.h"

void remove_duplicates(std::vector<rflcs_graph::match *> &matches);

void remove_dominated(std::vector<rflcs_graph::match *> &matches);

void update_graph_by_mdd(const instance &instance) {

    const auto matches = instance.is_solving_forward ? &instance.graph->matches : &instance.graph->reverse_matches;

    for (auto &match: *matches) {
        match.is_active = false;
        match.dom_succ_matches.clear();
        match.extension->succ_matches.clear();
    }

    for (const auto &level: instance.mdd->levels) {
        for (const auto &node: level->nodes) {
            const auto match = static_cast<rflcs_graph::match *>(node->associated_match);
            match->is_active = true;
            for (const auto succ_node: node->edges_out) {
                auto *succ_match = static_cast<rflcs_graph::match *>(succ_node->associated_match);
                match->dom_succ_matches.push_back(succ_match);
                match->extension->succ_matches.push_back(succ_match);
            }
        }
    }

    for (auto &match: *matches) {
        remove_duplicates(match.dom_succ_matches);
        remove_dominated(match.dom_succ_matches);
        remove_duplicates(match.extension->succ_matches);
    }

    for (auto &match: *matches) {
        for (const auto dom_succ_match: match.dom_succ_matches) {
            dom_succ_match->extension->dom_pred_matches.push_back(&match);
        }
        for (const auto succ_match: match.extension->succ_matches) {
            succ_match->extension->pred_matches.push_back(&match);
        }
    }
}

void remove_duplicates(std::vector<rflcs_graph::match *> &matches) {
    std::ranges::sort(matches);
    const auto [first, last] = std::ranges::unique(matches);
    matches.erase(first, last);
}

void remove_dominated(std::vector<rflcs_graph::match *> &matches) {
    std::ranges::sort(matches, [](const rflcs_graph::match *m1, const rflcs_graph::match *m2) {
        return m1->extension->position_1 < m2->extension->position_1;
    });
    const auto matches_copy = matches;
    matches.clear();
    int max_position_2 = std::numeric_limits<int>::max();
    for (auto match: matches_copy) {
        if (match->extension->position_2 < max_position_2) {
            matches.push_back(match);
            max_position_2 = match->extension->position_2;
        }
    }
}
