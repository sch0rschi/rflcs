#include "header/character_selection.hpp"
#include "header/initial_mdd.hpp"
#include "header/mdd_refinement.hpp"
#include "header/mdd_filter.hpp"
#include "../constants.hpp"
#include <boost/timer/progress_display.hpp>
#include <ranges>
#include <algorithm>
#include <absl/container/flat_hash_set.h>

#include "edge_utils.hpp"

void chaining_numbers(const mdd &mdd);

double calculate_greedy_score(const mdd &mdd, double initial_number_of_matches, double initial_number_of_graph_edges);

void update_characters_ordered_by_importance_mdd(
    std::vector<Character> &characters_ordered_by_importance,
    const instance &instance,
    const mdd &reduction_mdd,
    mdd_node_source &mdd_node_source,
    boost::timer::progress_display *progress) {
    chaining_numbers(reduction_mdd);

    auto matches_on_level = absl::flat_hash_set<void *>();
    auto static valid_edges = absl::flat_hash_set<long>();
    valid_edges.clear();
    for (auto const &level: reduction_mdd.levels | std::ranges::views::drop(1)) {
        for (const auto node: level->nodes) {
            matches_on_level.insert(node->associated_match);
            for (auto const &succ: node->edges_out) {
                valid_edges.insert(match_pair_to_edge_long_encoding(
                        static_cast<rflcs_graph::match *>(node->associated_match),
                        static_cast<rflcs_graph::match *>(succ->associated_match))
                );
            }
        }
    }
    const unsigned long initial_number_of_matches = matches_on_level.size();

    auto greedy_scores = std::vector<double>(constants::alphabet_size);
    for (const auto split_character: characters_ordered_by_importance) {
        if (temporaries::chaining_numbers[split_character] > 1) {
            std::unique_ptr<mdd> mdd_character_selection = mdd::copy_mdd(reduction_mdd, mdd_node_source);
            refine_mdd(*mdd_character_selection, split_character, mdd_node_source);
            filter_mdd(instance, *mdd_character_selection, mdd_node_source);
            greedy_scores[split_character] = calculate_greedy_score(
                *mdd_character_selection,
                static_cast<double>(initial_number_of_matches),
                static_cast<double>(valid_edges.size())
            );
            mdd_character_selection->deconstruct(mdd_node_source);
        } else {
            greedy_scores[split_character] = 0;
        }

        if (progress != nullptr) {
            ++*progress;
        }
    }
    std::ranges::stable_sort(characters_ordered_by_importance,
                             [&greedy_scores](const Character character_1, const Character character_2) {
                                 return greedy_scores[character_1] > greedy_scores[character_2];
                             });
}

double calculate_greedy_score(const mdd &mdd,
                              const double initial_number_of_matches,
                              const double initial_number_of_graph_edges
) {
    auto number_of_mdd_nodes = 0.0;
    auto static matches_on_level = absl::flat_hash_set<void *>();
    auto static matches = absl::flat_hash_set<void *>();
    matches.clear();
    auto static valid_edges = absl::flat_hash_set<long>();
    valid_edges.clear();
    auto number_of_mdd_edges = 0.0;
    auto max_in_edges = 0.0;
    for (const auto &level: mdd.levels | std::ranges::views::drop(1)) {
        matches_on_level.clear();
        for (const auto node: level->nodes) {
            matches_on_level.insert(node->associated_match);
            number_of_mdd_nodes++;
            number_of_mdd_edges += static_cast<double>(node->edges_in.size());
            max_in_edges = std::max(max_in_edges, static_cast<double>(node->edges_in.size()));
            for (const auto *succ: node->edges_out) {
                valid_edges.insert(match_pair_to_edge_long_encoding(
                        static_cast<rflcs_graph::match *>(node->associated_match),
                        static_cast<rflcs_graph::match *>(succ->associated_match))
                );
            }
        }
    }

    if (number_of_mdd_nodes == 0 || number_of_mdd_edges == 0) {
        return std::numeric_limits<double>::max();
    }


    const double match_delta = initial_number_of_matches - static_cast<double>(matches.size());
    const double graph_edge_delta = initial_number_of_graph_edges - static_cast<double>(valid_edges.size());
    return (1 + match_delta)
           * (1 + graph_edge_delta)
           / number_of_mdd_nodes
           / number_of_mdd_edges
           / max_in_edges;
}

void chaining_numbers(const mdd &mdd) {
    absl::flat_hash_map<node*, std::vector<int>> sequences_character_counter;
    for (const auto node: mdd.levels.back()->nodes) {
        sequences_character_counter[node] = std::vector<int>(constants::alphabet_size);
        std::ranges::fill(sequences_character_counter[node], 0);
        sequences_character_counter[node].at(node->character) = 1;
    }

    for (int level_index = static_cast<int>(mdd.levels.size()) - 2; level_index >= 0; level_index--) {
        for (const auto &level = mdd.levels.at(level_index); const auto node: level->nodes) {
            sequences_character_counter[node] = std::vector<int>(constants::alphabet_size);
            std::ranges::fill(sequences_character_counter[node], 0);
            for (const auto succ: node->edges_out) {
                std::ranges::transform(
                    sequences_character_counter[node],
                    sequences_character_counter[succ],
                    sequences_character_counter[node].begin(),
                    [](const int first, const int second) {
                        return std::max(first, second);
                    });
                sequences_character_counter[node].at(succ->character) = std::max(
                    sequences_character_counter[node].at(succ->character),
                    sequences_character_counter[succ].at(succ->character) + 1);
            }
        }
    }

    const auto root = *mdd.levels.front()->nodes.begin();
    temporaries::chaining_numbers = sequences_character_counter[root];
}
