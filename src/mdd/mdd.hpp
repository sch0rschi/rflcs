#pragma once

#include "mdd_node_source.hpp"
#include "mdd_levels.hpp"

#include <memory>

#include "absl/container/flat_hash_map.h"

struct mdd {
    levels_type levels;

    void deconstruct(mdd_node_source &mdd_node_source) const {
        for (const auto &level: levels) {
            for (auto *node: level->nodes) {
                mdd_node_source.clear_node(node);
            }
        }
    }

    static std::unique_ptr<mdd> copy_mdd(const mdd &original_mdd, mdd_node_source &mdd_node_source);
};

inline std::unique_ptr<mdd> mdd::copy_mdd(const mdd &original_mdd, mdd_node_source &mdd_node_source) {
    auto copy_mdd = std::make_unique<mdd>();
    copy_mdd->levels = levels_type();
    static auto original_node_map = absl::flat_hash_map<node*, node*>();
    original_node_map.clear();
    for (const auto &original_level: original_mdd.levels) {
        auto &[copy_nodes, copy_depth, _p] = *copy_mdd->levels.emplace_back(std::make_unique<level_type>());
        copy_depth = original_level->depth;
        copy_nodes.reserve(copy_nodes.size());
        for (const auto original_node: original_level->nodes) {
            node *copy_node = mdd_node_source.get_copy_of_old_node_with_copy_helper(original_node);
            original_node_map[original_node] = copy_node;
            copy_nodes.push_back(copy_node);
            for (const auto original_pred: original_node->edges_in) {
                original_node_map[original_pred]->link_pred_to_succ(copy_node);
                original_node_map[original_pred]->needs_update_from_succ = false;
            }
            copy_node->needs_update_from_pred = false;
            copy_node->needs_update_from_succ = false;
        }
    }
    return copy_mdd;
}

