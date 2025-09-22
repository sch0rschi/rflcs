#include "header/initial_mdd.hpp"
#include "header/domination_utils.hpp"
#include "header/character_bound_utils.hpp"
#include "mdd.hpp"
#include "../instance.hpp"
#include "../graph/graph.hpp"

#include <memory>
#include <vector>
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

std::unique_ptr<mdd> create_initial_mdd(const instance &instance, const bool forward) {
    auto mdd = std::make_unique<struct mdd>();
    mdd->levels = levels_type();

    instance.mdd_node_source->sort_cache();

    auto &[root_level, first_level_depth, _rp] = *mdd->levels.emplace_back(std::make_unique<level_type>());
    const auto root_node = instance.mdd_node_source->new_node();
    root_node->is_active = true;
    auto &root_match = forward ? instance.graph->matches.front() : instance.graph->reverse_matches.front();
    root_node->associated_match = &root_match;
    root_node->character = root_match.character;
    root_node->position_1 = root_match.extension->position_1;
    root_node->position_2 = root_match.extension->position_2;
    root_node->characters_on_paths_to_root = Character_set();
    root_node->characters_on_all_paths_to_root = Character_set();
    root_node->characters_on_paths_to_some_sink = root_match.extension->available_characters;
    root_node->characters_on_all_paths_to_lower_bound_levels = Character_set();
    root_node->upper_bound_down = temporaries::upper_bound;

    root_level.push_back(root_node);

    auto match_to_node_map = absl::flat_hash_map<rflcs_graph::match *, node *>();
    while (!mdd->levels.back()->nodes.empty() && mdd->levels.back()->depth<temporaries::upper_bound) {
        const auto &[current_nodes, current_depth, _p] = *mdd->levels.back();
        auto &[next_nodes, next_depth, _np] = *mdd->levels.emplace_back(std::make_unique<level_type>());
        next_depth = current_depth + 1;
        match_to_node_map.clear();
        for (const auto pred_node: current_nodes) {
            int min_position_2 = INT_MAX;
            auto pred_node_match = static_cast<rflcs_graph::match*>(pred_node->associated_match);
            int min_positions_2_size = 0;
            static auto min_positions_2 = std::vector<int>(constants::alphabet_size);
            for (auto succ_match: pred_node_match->extension->succ_matches) {
                const bool not_dominated = !dominated_by_some_available_but_unused_character(
                    succ_match->extension->position_2, current_depth, min_positions_2, min_positions_2_size);
                if (succ_match->character<constants::alphabet_size
                                          && next_depth <= succ_match->reversed->upper_bound
                                          && succ_match->extension->position_2<min_position_2
                                                                              && pred_node->
                                                                              characters_on_paths_to_some_sink.test(
                                                                                  succ_match->character)
                                                                              && next_depth + succ_match->upper_bound>
                    temporaries::lower_bound
                    && not_dominated
                    && are_enough_characters_available(temporaries::lower_bound,
                                                       next_depth,
                                                       pred_node_match->reversed->extension->
                                                       available_characters,
                                                       succ_match->extension->available_characters)
                ) {
                    if (!pred_node_match->reversed->extension->available_characters.test(
                        succ_match->character)) {
                        min_position_2 = std::min(min_position_2, succ_match->extension->position_2);
                    } else {
                        add_position_2_to_maybe_min_pos_2(min_positions_2, succ_match->extension->position_2, min_positions_2_size, current_depth);
                    }
                    if (succ_match->is_active) {
                        node *succ_node;
                        if (match_to_node_map.contains(succ_match)) {
                            succ_node = match_to_node_map.at(succ_match);
                        } else {
                            succ_node = instance.mdd_node_source->get_new_node_with_match(*succ_match);
                            match_to_node_map.insert({succ_match, succ_node});
                            next_nodes.push_back(succ_node);
                        }
                        pred_node->link_pred_to_succ(succ_node);
                    }
                }
            }
        }
    }
    while (mdd->levels.back()->nodes.empty()) {
        mdd->levels.pop_back();
    }

    return mdd;
}

void prune_by_flat_mdd(shared_object *shared_object, const mdd &mdd, mdd_node_source &mdd_node_source) {
    auto *current_pointer = std::bit_cast<std::byte *>(&shared_object->flat_levels);

    auto static valid_matches_sets = std::vector<absl::flat_hash_set<void *> >(mdd.levels.size());
    valid_matches_sets.resize(mdd.levels.size());
    for (auto &valid_matches: valid_matches_sets) {
        valid_matches.clear();
    }

    static auto valid_edges_sets =
            std::vector<absl::flat_hash_set<std::pair<void *, void*> > >(mdd.levels.size());
    valid_edges_sets.resize(mdd.levels.size());
    for (auto &valid_edges: valid_edges_sets) {
        valid_edges.clear();
    }

    for (size_t level_index = 0; level_index < shared_object->num_levels; ++level_index) {
        auto &current_level_valid_matches = valid_matches_sets.at(level_index);
        auto &current_level_valid_edges = valid_edges_sets.at(level_index);
        const auto *flat_level = std::bit_cast<struct flat_level *>(current_pointer);
        current_pointer += sizeof(struct flat_level);

        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            auto flat_node = std::bit_cast<struct flat_node *>(current_pointer);
            auto *match = std::bit_cast<rflcs_graph::match *>(flat_node->match_ptr);
            if (flat_node->is_active) {
                current_level_valid_matches.insert(match);
            }
            current_pointer += sizeof(struct flat_node);

            for (size_t edge_index = 0; edge_index < flat_node->num_edges_out; ++edge_index) {
                if (auto flat_edge = std::bit_cast<struct flat_edge *>(current_pointer); flat_edge->is_active) {
                    auto *to = std::bit_cast<rflcs_graph::match *>(flat_edge->edge_node->match_ptr);
                    current_level_valid_edges.emplace(match, to);
                }
                current_pointer += sizeof(flat_edge);
            }
        }
    }

    for (const auto &level: mdd.levels) {
        const auto &current_valid_matches = valid_matches_sets.at(level->depth);
        const auto &current_valid_edges = valid_edges_sets.at(level->depth);
        for (std::vector nodes(level->nodes.begin(), level->nodes.end()); const auto node: nodes) {
            if (!current_valid_matches.contains(node->associated_match)) {
                std::erase(level->nodes, node);
                mdd_node_source.clear_node(node);
            } else {
                for (std::vector succs(node->edges_out.begin(), node->edges_out.end()); const auto succ: succs) {
                    if (!current_valid_edges.contains({node->associated_match, succ->associated_match})) {
                        node->unlink_pred_from_succ(succ);
                    }
                }
            }
        }
    }
}
