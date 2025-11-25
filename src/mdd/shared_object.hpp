#pragma once

#include "mdd.hpp"

struct instance;
struct shared_object;

size_t calculate_shared_object_size(const mdd& data);

void serialize_initial_mdd(const mdd& mdd, shared_object* shared_object);

int get_active_match_count(shared_object* flat_mdd);

int get_active_edge_count(shared_object* flat_mdd);

struct flat_node;

struct flat_edge {
    flat_node* edge_node;
    bool is_active;
};

struct flat_node {
    void* match_ptr;
    Character character;
    int position_2;
    bool is_active;
    size_t num_edges_out;
    flat_edge edges_out[];
};

struct flat_level {
    int depth;
    size_t num_nodes;
    flat_node nodes[];
};

struct shared_object {
    int upper_bound = std::numeric_limits<int>::max();
    int active_match_count = std::numeric_limits<int>::max();
    int number_of_refined_characters = 0;
    int refinement_round = 1;
    bool is_mdd_reduction_complete = false;
    size_t num_levels = std::numeric_limits<int>::max();
    flat_level flat_levels[];
};
