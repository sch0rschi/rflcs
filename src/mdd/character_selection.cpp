#include "header/character_selection.hpp"
#include "header/initial_mdd.hpp"
#include "header/mdd_refinement.hpp"
#include "header/mdd_filter.hpp"
#include <boost/timer/progress_display.hpp>
#include <ranges>
#include <algorithm>
#include <absl/container/flat_hash_set.h>

#include "edge_utils.hpp"

void chaining_numbers(const mdd &mdd, const character_counters_source &character_counters_source);

double calculate_greedy_score(const mdd &mdd, double initial_number_of_matches, double initial_number_of_graph_edges);

void update_characters_ordered_by_importance_mdd(
    std::vector<Character> &characters_ordered_by_importance,
    instance &instance,
    const mdd &reduction_mdd,
    mdd_node_source &mdd_node_source,
    const character_counters_source &character_counters_source,
    boost::timer::progress_display *progress) {
    chaining_numbers(reduction_mdd, character_counters_source);

    auto matches_on_level = absl::flat_hash_set<rflcs_graph::match *>();
    auto static valid_edges = absl::flat_hash_set<long>();
    valid_edges.clear();
    for (auto const &level: reduction_mdd.levels | std::ranges::views::drop(1)) {
        for (const auto node: level->nodes) {
            matches_on_level.insert(node->match);
            for (auto const &succ: node->edges_out) {
                valid_edges.insert(match_pair_to_edge_long_encoding(node->match, succ->match));
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
    auto static matches_on_level = absl::flat_hash_set<rflcs_graph::match *>();
    auto static matches = absl::flat_hash_set<rflcs_graph::match *>();
    matches.clear();
    auto static valid_edges = absl::flat_hash_set<long>();
    valid_edges.clear();
    auto number_of_mdd_edges = 0.0;
    auto max_in_edges = 0.0;
    for (const auto &level: mdd.levels | std::ranges::views::drop(1)) {
        matches_on_level.clear();
        for (const auto node: level->nodes) {
            matches_on_level.insert(node->match);
            number_of_mdd_nodes++;
            number_of_mdd_edges += static_cast<double>(node->edges_in.size());
            max_in_edges = std::max(max_in_edges, static_cast<double>(node->edges_in.size()));
            for (const auto *succ: node->edges_out) {
                valid_edges.insert(match_pair_to_edge_long_encoding(node->match, succ->match));
            }
        }
    }

    if (number_of_mdd_nodes == 0 || number_of_mdd_edges == 0) {
        return MAXFLOAT;
    }


    const double match_delta = initial_number_of_matches - static_cast<double>(matches.size());
    const double graph_edge_delta = initial_number_of_graph_edges - static_cast<double>(valid_edges.size());
    return match_delta
           * graph_edge_delta
           / number_of_mdd_nodes
           / number_of_mdd_edges
           / max_in_edges;
}

void chaining_numbers(const mdd &mdd, const character_counters_source &character_counters_source) {
    for (const auto node: mdd.levels.back()->nodes) {
        node->sequences_character_counter = character_counters_source.get_counter();
        std::ranges::fill(*node->sequences_character_counter, 0);
        node->sequences_character_counter->at(node->match->character) = 1;
    }

    for (int level_index = static_cast<int>(mdd.levels.size()) - 2; level_index >= 0; level_index--) {
        for (const auto &level = mdd.levels.at(level_index); const auto node: level->nodes) {
            node->sequences_character_counter = character_counters_source.get_counter();
            std::ranges::fill(*node->sequences_character_counter, 0);
            for (const auto succ: node->edges_out) {
                std::ranges::transform(
                    *node->sequences_character_counter,
                    *succ->sequences_character_counter,
                    node->sequences_character_counter->begin(),
                    [](const int first, const int second) {
                        return std::max(first, second);
                    });
                node->sequences_character_counter->at(succ->match->character) = std::max(
                    node->sequences_character_counter->at(succ->match->character),
                    succ->sequences_character_counter->at(succ->match->character) + 1);
            }
        }

        for (const auto &level = mdd.levels.at(level_index + 1); const auto node: level->nodes) {
            character_counters_source.return_counter(node->sequences_character_counter);
            node->sequences_character_counter = nullptr;
        }
    }

    const auto root = *mdd.levels.front()->nodes.begin();
    temporaries::chaining_numbers = *root->sequences_character_counter;
    character_counters_source.return_counter(root->sequences_character_counter);
    root->sequences_character_counter = nullptr;
}
