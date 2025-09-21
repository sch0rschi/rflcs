#include "header/reduce_graph.hpp"
#include "graph.hpp"

#include <vector>

auto bad_edge(const rflcs_graph::match &current_match,
              const rflcs_graph::match &target_match) -> bool;

auto bad_edge_with_exception(
    const rflcs_graph::match &current_match,
    const rflcs_graph::match &target_match,
    const rflcs_graph::match &exception) -> bool;

auto reduce_dominating_succ_edges(
    rflcs_graph::match &rev_root,
    rflcs_graph::match &current_match) -> void;

auto reduce_succ_edges(rflcs_graph::match &current_match) -> void;

auto reduce_edges(std::vector<rflcs_graph::match> &matches) -> void;

auto reduce_graph(rflcs_graph::graph &graph) -> void {
    reduce_edges(graph.matches);
    reduce_edges(graph.reverse_matches);
}

auto reduce_edges(std::vector<rflcs_graph::match> &matches) -> void {
    for (auto &current_match: matches) {
        if (current_match.is_active) {
            reduce_succ_edges(current_match);
            reduce_dominating_succ_edges(matches.back(), current_match);
        }
    }
}

auto reduce_dominating_succ_edges(
    rflcs_graph::match &rev_root,
    rflcs_graph::match &current_match) -> void {
    std::erase_if(current_match.dom_succ_matches,
                  [&current_match, &rev_root](const rflcs_graph::match *succ_match) {
                      return bad_edge_with_exception(current_match, *succ_match, rev_root);
                  });

    if (current_match.dom_succ_matches.empty()) {
        current_match.dom_succ_matches.push_back(&rev_root);
    }
}

auto reduce_succ_edges(rflcs_graph::match &current_match) -> void {
    std::erase_if(current_match.extension->succ_matches,
                  [&current_match](const rflcs_graph::match *succ_match) {
                      return bad_edge(current_match, *succ_match);
                  });
}

inline auto bad_edge(
    const rflcs_graph::match &current_match,
    const rflcs_graph::match &target_match) -> bool {
    if (!target_match.is_active) {
        return true;
    }
    if (current_match.reversed->upper_bound + target_match.upper_bound <= temporaries::lower_bound) {
        return true;
    }
    if (current_match.reversed->upper_bound < target_match.extension->transient_match_domination_number) {
        return true;
    }
    temporaries::temp_character_set_1 = current_match.reversed->extension->available_characters;
    temporaries::temp_character_set_1 |= target_match.extension->available_characters;
    return static_cast<int>(temporaries::temp_character_set_1.count()) <= temporaries::lower_bound;
}

auto bad_edge_with_exception(
    const rflcs_graph::match &current_match,
    const rflcs_graph::match &target_match,
    const rflcs_graph::match &exception) -> bool {
    if (&target_match == &exception) {
        return false;
    }
    return bad_edge(current_match, target_match);
}
