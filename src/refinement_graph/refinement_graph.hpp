#ifndef REFINEMENT_GRAPH_HPP
#define REFINEMENT_GRAPH_HPP

#include <climits>

#include "boost/dynamic_bitset/dynamic_bitset.hpp"
#include "header/graphiz_utils.hpp"
#include "../types.hpp"

struct refinement_node;

struct refinement_match {
    character_type character = SHRT_MAX;
    int position_1 = -1;
    int position_2 = -1;
    std::vector<refinement_node *> refinement_nodes = std::vector<refinement_node *>();
};

struct refinement_node {
    refinement_match *refinement_match;
    int upper_bound_up = INT_MAX;
    int upper_bound_down = INT_MAX;
    int upper_bound = INT_MAX;
    boost::dynamic_bitset<> characters_on_paths_to_root;
    boost::dynamic_bitset<> characters_on_all_paths_to_root;
    boost::dynamic_bitset<> characters_on_paths_to_some_sink;
    boost::dynamic_bitset<> characters_on_all_paths_to_lower_bound_length;
    std::vector<refinement_node *> predecessors = std::vector<refinement_node *>();
    std::vector<refinement_node *> successors = std::vector<refinement_node *>();

    refinement_node *split(character_type refinement_character) {
        const auto new_refinement_node = new refinement_node();
        new_refinement_node->refinement_match = this->refinement_match;
        new_refinement_node->upper_bound_up = this->upper_bound_up;
        new_refinement_node->upper_bound_down = this->upper_bound_down;
        new_refinement_node->characters_on_paths_to_root = this->characters_on_paths_to_root;
        new_refinement_node->characters_on_all_paths_to_root = this->characters_on_all_paths_to_root;
        new_refinement_node->characters_on_paths_to_some_sink = this->characters_on_paths_to_some_sink;
        new_refinement_node->characters_on_all_paths_to_lower_bound_length = this->
                characters_on_all_paths_to_lower_bound_length;

        this->characters_on_paths_to_some_sink.reset(refinement_character);
        this->characters_on_all_paths_to_root.set(refinement_character);
        new_refinement_node->characters_on_paths_to_root.reset(refinement_character);

        auto start_of_move = std::partition(
            this->predecessors.begin(),
            this->predecessors.end(),
            [refinement_character](const auto &predecessor) {
                return predecessor->characters_on_all_paths_to_root.test(refinement_character);
            }
        );

        const auto amount_to_move = std::distance(start_of_move, this->predecessors.end());
        new_refinement_node->predecessors.reserve(amount_to_move);

        new_refinement_node->predecessors.insert(
            new_refinement_node->predecessors.end(),
            std::make_move_iterator(start_of_move),
            std::make_move_iterator(this->predecessors.end())
        );
        this->predecessors.erase(start_of_move, this->predecessors.end());

        for (auto *predecessor_not_previous: new_refinement_node->predecessors) {
            std::ranges::replace(predecessor_not_previous->successors, this, new_refinement_node);
        }

        new_refinement_node->successors = this->successors;
        for (auto *successor: this->successors) {
            successor->predecessors.push_back(new_refinement_node);
        }

        this->refinement_match->refinement_nodes.push_back(new_refinement_node);

        return new refinement_node;
    }

    void prune() {
        std::erase(this->refinement_match->refinement_nodes, this);
        for (const auto succ: this->successors) {
            std::erase(succ->predecessors, this);
        }
        for (const auto pred : this->predecessors) {
            std::erase(pred->successors, this);
        }
    }

    void unlink_from_succ(refinement_node* succ) {
        std::erase(succ->predecessors, this);
        std::erase(this->successors, succ);
    }
};

#endif
