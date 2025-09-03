#include "shared_object.hpp"
#include "mdd.hpp"
#include <map>
#include <absl/container/flat_hash_set.h>

size_t calculate_flat_array_size(const mdd& data) {
    size_t size = 0;
    size += sizeof(shared_object);
    for (const auto& level : data.levels) {
        size += sizeof(flat_level);
        for (const auto node : level->nodes) {
            size += sizeof(flat_node);
            size += node->edges_out.size() * sizeof(flat_edge);
        }
    }
    return size;
}

void serialize_initial_mdd(const mdd& mdd, shared_object* shared_object) {
    auto level_node_to_match = std::map<std::pair<int, rflcs_graph::match*>, flat_node*>();
    auto* current_pointer = std::bit_cast<std::byte *>(shared_object);
    shared_object->num_levels = mdd.levels.size();
    current_pointer += sizeof(struct shared_object);

    for (const auto& level : mdd.levels) {
        auto flat_level = std::bit_cast<struct flat_level*>(current_pointer);
        flat_level->depth = level->depth;
        flat_level->num_nodes = level->nodes.size();
        current_pointer += sizeof(struct flat_level);

        for (const auto node : level->nodes) {
            auto flat_node = std::bit_cast<struct flat_node*>(current_pointer);
            flat_node->match_ptr = node->match;
            flat_node->character = node->match->character;
            flat_node->position_2 = node->match->extension.position_2;
            flat_node->is_active = true;
            flat_node->num_edges_out = node->edges_out.size();
            level_node_to_match.try_emplace(std::make_pair(level->depth, node->match), flat_node);
            current_pointer += sizeof(struct flat_node);
            current_pointer += sizeof(flat_edge) * node->edges_out.size();
        }
    }

    current_pointer = std::bit_cast<std::byte *>(shared_object);
    current_pointer += sizeof(struct shared_object);

    for (const auto& level : mdd.levels) {
        current_pointer += sizeof(flat_level);

        for (const auto node : level->nodes) {
            current_pointer += sizeof(flat_node);

            for (const auto edge : node->edges_out) {
                auto flat_edge = std::bit_cast<struct flat_edge*>(current_pointer);
                flat_edge->edge_node = level_node_to_match[std::make_pair(level->depth + 1, edge->match)];
                flat_edge->is_active = true;
                current_pointer += sizeof(struct flat_edge);
            }
        }
    }
    shared_object->active_match_count = get_active_match_count(shared_object);
}

int get_active_match_count(shared_object* flat_mdd) {
    auto static matches = absl::flat_hash_set<void*>();
    matches.clear();
    auto current_pointer =  std::bit_cast<std::byte *>(flat_mdd);
    current_pointer += sizeof(shared_object);
    for (size_t level_index = 0; level_index < flat_mdd->num_levels; ++level_index) {
        const auto *flat_level = std::bit_cast<struct flat_level*>(current_pointer);
        current_pointer += sizeof(struct flat_level);
        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            auto flat_node = std::bit_cast<struct flat_node*>(current_pointer);
            if(flat_node->is_active) {
                matches.insert(flat_node->match_ptr);
            }
            current_pointer += sizeof(struct flat_node);
            current_pointer += flat_node->num_edges_out * sizeof(flat_edge);
        }
    }
    return static_cast<int>(matches.size()) - 1;
}

int get_active_edge_count(shared_object* flat_mdd) {
    int active_edge_count = 0;
    auto current_pointer = std::bit_cast<std::byte *>(&flat_mdd->flat_levels);
    for (size_t level_index = 0; level_index < flat_mdd->num_levels; ++level_index) {
        const auto *flat_level = std::bit_cast<struct flat_level*>(current_pointer);
        current_pointer += sizeof(struct flat_level);
        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            const auto *flat_node = std::bit_cast<struct flat_node*>(current_pointer);
            current_pointer += sizeof(struct flat_node);
            for (size_t edge_index = 0; edge_index < flat_node->num_edges_out; ++edge_index) {
                if(const auto *flat_edge = std::bit_cast<struct flat_edge*>(current_pointer); flat_edge->is_active) {
                    active_edge_count++;
                }
                current_pointer += sizeof(flat_edge);
            }
        }
    }
    return active_edge_count;
}
