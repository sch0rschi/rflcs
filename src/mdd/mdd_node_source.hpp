#pragma once

#include "mdd_node.hpp"
#include <algorithm>

struct mdd_node_source {

private:
    std::vector<node *> cache= std::vector<node *>();

public:
    void clear_node(node *node) {
        node->clear();
        cache.push_back(node);
    }

    void sort_cache() {
        std::ranges::sort(cache);
    }

    [[nodiscard]] node* new_node() {
        if(!cache.empty()) {
            const auto node = cache.back();
            cache.pop_back();
            return node;
        }
        return new node();
    }

    [[nodiscard]] node *get_new_node_with_match(rflcs_graph::match &match) {

        node *fresh_node;
        if (cache.empty()) {
            fresh_node = new_node();
            fresh_node->characters_on_paths_to_root = match.extension.reversed->extension.available_characters;
            fresh_node->characters_on_all_paths_to_root = Character_set();
            fresh_node->characters_on_paths_to_some_sink = match.extension.available_characters;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = Character_set();
        } else {
            fresh_node = cache.back();
            cache.pop_back();
            fresh_node->characters_on_paths_to_root = match.extension.reversed->extension.available_characters;
            fresh_node->characters_on_all_paths_to_root.reset();
            fresh_node->characters_on_paths_to_some_sink = match.extension.available_characters;
            fresh_node->characters_on_all_paths_to_lower_bound_levels.reset();
        }
        fresh_node->is_active = true;
        fresh_node->associated_match = &match;
        fresh_node->character = match.character;
        fresh_node->position_1 = match.extension.position_1;
        fresh_node->position_2 = match.extension.position_2;

        fresh_node->characters_on_paths_to_root.set(fresh_node->character);
        fresh_node->characters_on_all_paths_to_root.set(fresh_node->character);
        fresh_node->characters_on_paths_to_some_sink.reset(match.character);

        fresh_node->upper_bound_down = match.upper_bound;
        fresh_node->needs_update_from_pred = true;
        fresh_node->needs_update_from_succ = true;
        return fresh_node;
    }

    [[nodiscard]] node *get_copy_of_old_node(const node &old_node) {
        node *fresh_node;
        if (cache.empty()) {
            fresh_node = new_node();
            fresh_node->characters_on_paths_to_root = old_node.characters_on_paths_to_root;
            fresh_node->characters_on_all_paths_to_root = old_node.characters_on_all_paths_to_root;
            fresh_node->characters_on_paths_to_some_sink = old_node.characters_on_paths_to_some_sink;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = old_node.characters_on_all_paths_to_lower_bound_levels;
        } else {
            fresh_node = cache.back();
            cache.pop_back();
            fresh_node->characters_on_paths_to_root = old_node.characters_on_paths_to_root;
            fresh_node->characters_on_all_paths_to_root = old_node.characters_on_all_paths_to_root;
            fresh_node->characters_on_paths_to_some_sink = old_node.characters_on_paths_to_some_sink;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = old_node.characters_on_all_paths_to_lower_bound_levels;
        }
        fresh_node->is_active = true;
        fresh_node->associated_match = old_node.associated_match;
        fresh_node->character = old_node.character;
        fresh_node->position_1 = old_node.position_1;
        fresh_node->position_2 = old_node.position_2;
        fresh_node->upper_bound_down = old_node.upper_bound_down;

        fresh_node->needs_update_from_pred = true;
        fresh_node->needs_update_from_succ = true;
        return fresh_node;
    }

    [[nodiscard]] node *get_copy_of_old_node_with_copy_helper(const node *old_node) {
        node *fresh_node;
        if (cache.empty()) {
            fresh_node = new_node();
            fresh_node->characters_on_paths_to_root = old_node->characters_on_paths_to_root;
            fresh_node->characters_on_all_paths_to_root = old_node->characters_on_all_paths_to_root;
            fresh_node->characters_on_paths_to_some_sink = old_node->characters_on_paths_to_some_sink;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = old_node->characters_on_all_paths_to_lower_bound_levels;
        } else {
            fresh_node = cache.back();
            cache.pop_back();
            fresh_node->characters_on_paths_to_root = old_node->characters_on_paths_to_root;
            fresh_node->characters_on_all_paths_to_root = old_node->characters_on_all_paths_to_root;
            fresh_node->characters_on_paths_to_some_sink = old_node->characters_on_paths_to_some_sink;
            fresh_node->characters_on_all_paths_to_lower_bound_levels = old_node->characters_on_all_paths_to_lower_bound_levels;
        }
        fresh_node->is_active = true;
        fresh_node->associated_match = old_node->associated_match;
        fresh_node->character = old_node->character;
        fresh_node->position_1 = old_node->position_1;
        fresh_node->position_2 = old_node->position_2;
        fresh_node->upper_bound_down = old_node->upper_bound_down;

        fresh_node->needs_update_from_pred = false;
        fresh_node->needs_update_from_succ = false;

        return fresh_node;
    }

    ~mdd_node_source() {
        cache.clear();
    }
};
