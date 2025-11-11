#include "sequence_enumeration_solver.hpp"
#include "../constants.hpp"

void set_solution(instance &instance, const std::vector<flat_node *> &stack, int pointer);

void solve_enumeration(instance &instance) {

    auto *current_pointer = std::bit_cast<std::byte *>(&instance.shared_object->flat_levels);
    for (size_t level_index = 0; level_index < instance.shared_object->num_levels; ++level_index) {
        const auto flat_level = std::bit_cast<const struct flat_level *>(current_pointer);
        current_pointer += sizeof(struct flat_level);
        for (size_t node_index = 0; node_index < flat_level->num_nodes; ++node_index) {
            auto flat_node = std::bit_cast<struct flat_node *>(current_pointer);
            flat_node->is_active = false;
            current_pointer += sizeof(struct flat_node);
            auto *edges = std::bit_cast<flat_edge *>(current_pointer);
            std::sort(edges, edges + flat_node->num_edges_out, [](const flat_edge &edge_1, const flat_edge &edge_2) {
                const auto match_1 = static_cast<rflcs_graph::match *>(edge_1.edge_node->match_ptr);
                const auto match_2 = static_cast<rflcs_graph::match *>(edge_2.edge_node->match_ptr);
                return std::tie(match_1->extension->position_1,
                                match_1->extension->position_2)
                       <
                       std::tie(match_2->extension->position_1,
                                match_2->extension->position_2);
            });
            current_pointer += flat_node->num_edges_out * sizeof(flat_edge);
        }
    }

    current_pointer = std::bit_cast<std::byte *>(&instance.shared_object->flat_levels);
    current_pointer += sizeof(flat_level);
    const auto *root_node = std::bit_cast<flat_node *>(current_pointer);

    auto stack_vector = std::vector<flat_node *>();
    stack_vector.reserve(constants::alphabet_size * temporaries::upper_bound);

    const auto stack = stack_vector.data();
    int stack_pointer = -1;
    for (size_t i = 0; i < root_node->num_edges_out; i++) {
        stack[++stack_pointer] = root_node->edges_out[i].edge_node;
    }

    int depth = 0;
    auto used_characters = Character_set();
    used_characters.reset();
    flat_node *current_node;
    while (stack_pointer >= 0) {
        current_node = stack[stack_pointer];
        if (current_node->is_active) {
            current_node->is_active = false;
            used_characters.reset(current_node->character);
            --depth;
            --stack_pointer;
        } else {
            current_node->is_active = true;
            used_characters.set(current_node->character);
            ++depth;
            if (depth > temporaries::lower_bound) {
                set_solution(instance, stack_vector, stack_pointer);
                temporaries::lower_bound = depth;
                if(temporaries::lower_bound >= temporaries::upper_bound) {
                    instance.is_valid_solution = true;
                    return;
                }
            }
            int position_2_min = std::numeric_limits<int>::max();
            for (size_t i = 0; i < current_node->num_edges_out; ++i) {
                if (flat_node *potential_node = current_node->edges_out[i].edge_node;
                    position_2_min > potential_node->position_2 && !used_characters.test(potential_node->character)) {
                    stack[++stack_pointer] = potential_node;
                    position_2_min = potential_node->position_2;
                }
            }
        }
    }
    instance.is_valid_solution = true;
}

void set_solution(instance &instance, const std::vector<flat_node *> &stack, const int pointer) {
    instance.solution.clear();
    for (int i = 0; i <= pointer; i++) {
        if (const auto *node = stack[i]; node->is_active) {
            instance.solution.push_back(node->character);
        }
    }
}
