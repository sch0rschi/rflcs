#include "refinement_graph.hpp"

#include "header/graphiz_utils.hpp"
#include <fstream>

#include "match_loop_utils.h"
#include "../config.hpp"

std::string bitset_to_index_set(const boost::dynamic_bitset<> &character_set);

void refine_graph_by_character(const instance &instance, character_type refinement_character);

void filter_refinement_graph(const instance &instance);

void initialize_refinement_graph(instance &instance) {
    for (auto &match: instance.graph->matches
                      | active_match_filter
                      | std::views::reverse
                      | std::views::drop(1)) {
        auto new_node = new rflcs_graph::refinement_node();
        match.extension.initial_refinement_node = new_node;
        new_node->match = &match;
        new_node->character = match.character;

        new_node->characters_on_paths_to_root = match.extension.reversed->extension.available_characters;
        new_node->characters_on_all_paths_to_root = boost::dynamic_bitset<>(instance.alphabet_size);
        new_node->characters_on_paths_to_some_sink = match.extension.available_characters;
        new_node->characters_on_all_paths_to_lower_bound_length = boost::dynamic_bitset<>(instance.alphabet_size);
        if (match.character >= 0 && match.character < instance.alphabet_size) {
            new_node->characters_on_paths_to_root.set(match.character);
            new_node->characters_on_all_paths_to_root.set(match.character);
            new_node->characters_on_paths_to_some_sink.reset(match.character);
            new_node->characters_on_all_paths_to_lower_bound_length.reset(match.character);
        }

        match.nodes.push_back(new_node);
        for (const auto *succ: match.extension.succ_matches) {
            if (succ->character >= 0 && succ->character < instance.alphabet_size) {
                new_node->successors.push_back(succ->extension.initial_refinement_node);
                succ->extension.initial_refinement_node->predecessors.push_back(new_node);
            }
        }
        instance.active_match_pointers.push_back(&match);
    }
    std::ranges::reverse(instance.active_match_pointers);
    filter_refinement_graph(instance);
    write_refinement_graph(instance, "refinement_graph_initial.dot");
}

void graph_refinement(const instance &instance) {
    for (character_type refinement_character = 0;
         refinement_character < static_cast<character_type>(instance.alphabet_size); ++refinement_character) {
        refine_graph_by_character(instance, refinement_character);
        filter_refinement_graph(instance);
        if (IS_WRITING_DOT_FILE) {
            auto filename = std::string("refinement_graph") + std::to_string(refinement_character) + ".dot";
            write_refinement_graph(instance, filename);
        }
    }
}

void refine_graph_by_character(const instance &instance, const character_type refinement_character) {
    for (const auto match: instance.active_match_pointers) {
        for (const auto split_node_fixed_previous: std::vector(match->nodes)) {
            if (split_node_fixed_previous->characters_on_paths_to_root.test(refinement_character)
                && split_node_fixed_previous->characters_on_paths_to_some_sink.test(refinement_character)) {
                auto *split_node_exclude_previous = match->nodes.emplace_back(new rflcs_graph::refinement_node());
                split_node_exclude_previous->match = split_node_fixed_previous->match;
                split_node_exclude_previous->character = split_node_fixed_previous->character;
                split_node_exclude_previous->characters_on_paths_to_root = split_node_fixed_previous->
                        characters_on_paths_to_root;
                split_node_exclude_previous->characters_on_paths_to_root.reset(refinement_character);
                split_node_exclude_previous->characters_on_all_paths_to_root = split_node_fixed_previous->
                        characters_on_all_paths_to_root;
                split_node_exclude_previous->characters_on_paths_to_some_sink = split_node_fixed_previous->
                        characters_on_paths_to_some_sink;
                split_node_exclude_previous->characters_on_all_paths_to_lower_bound_length = split_node_fixed_previous->
                        characters_on_all_paths_to_lower_bound_length;
                split_node_exclude_previous->upper_bound_up = split_node_fixed_previous->upper_bound_up;
                split_node_exclude_previous->upper_bound_down = split_node_fixed_previous->upper_bound_down;

                split_node_fixed_previous->characters_on_paths_to_some_sink.reset(refinement_character);
                split_node_fixed_previous->characters_on_all_paths_to_root.set(refinement_character);

                split_node_exclude_previous->characters_on_paths_to_root.reset(refinement_character);

                write_refinement_graph(instance, "refinement_graph_current.dot");

                auto predecessors = split_node_fixed_previous->predecessors;
                split_node_fixed_previous->predecessors.clear();

                for (const auto predecessor: predecessors) {
                    if (predecessor->characters_on_all_paths_to_root.test(refinement_character)) {
                        split_node_fixed_previous->predecessors.push_back(predecessor);
                    } else {
                        split_node_exclude_previous->predecessors.push_back(predecessor);
                    }
                }

                write_refinement_graph(instance, "refinement_graph_current.dot");

                for (auto *predecessor_not_previous: split_node_exclude_previous->predecessors) {
                    std::ranges::replace(predecessor_not_previous->successors,
                                         split_node_fixed_previous,
                                         split_node_exclude_previous);
                }
                write_refinement_graph(instance, "refinement_graph_current.dot");

                split_node_exclude_previous->successors = split_node_fixed_previous->successors;
                for (auto *successor: split_node_fixed_previous->successors) {
                    successor->predecessors.push_back(split_node_exclude_previous);
                }
                write_refinement_graph(instance, "refinement_graph_current.dot");
    int i = 0;
            }
        }
    }
}

void filter_refinement_graph(const instance &instance) {
    for (const auto match: instance.active_match_pointers | std::views::drop(1)) {
        for (auto *node: std::vector(match->nodes)) {
            if (node->predecessors.empty()) {
                std::erase(match->nodes, node);
                for (auto *succ: node->successors) {
                    std::erase(succ->predecessors, node);
                }
            }
            for (auto successor: std::vector(node->successors)) {
                if (!node->characters_on_paths_to_some_sink.test(successor->character)) {
                    std::erase(successor->predecessors, node);
                    std::erase(node->successors, successor);
                }
            }
        }
    }
}

void write_refinement_graph(const instance &instance, const std::string &filename) {
    using Graph = boost::adjacency_list<
        boost::vecS,
        boost::vecS,
        boost::directedS,
        boost::property<boost::vertex_name_t, std::string> >;

    Graph graph;

    for (auto const match: instance.active_match_pointers | std::views::reverse) {
        for (auto const node: match->nodes) {
            node->vertex = boost::add_vertex(
                "(" + std::to_string(node->match->extension.position_1) + ", "
                + std::to_string(node->match->extension.position_2) + "), "
                + std::to_string(node->character) + "\n"
                + bitset_to_index_set(node->characters_on_paths_to_root) + " "
                + bitset_to_index_set(node->characters_on_paths_to_some_sink) + "\n"
                + bitset_to_index_set(node->characters_on_all_paths_to_root) + " "
                + bitset_to_index_set(node->characters_on_all_paths_to_lower_bound_length),
                graph);
            for (const auto succ: node->successors) {
                boost::add_edge(node->vertex, succ->vertex, graph);
            }
        }
    }

    for (auto const match: instance.active_match_pointers) {
        for (auto const node: match->nodes) {
            for (const auto predecessor: node->predecessors) {
                boost::add_edge(predecessor->vertex, node->vertex, graph);
            }
        }
    }

    std::ofstream fout(filename);
    write_graphviz(fout, graph, boost::make_label_writer(get(boost::vertex_name, graph)));
}

std::string bitset_to_index_set(const boost::dynamic_bitset<> &character_set) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (std::size_t i = 0; i < character_set.size(); ++i) {
        if (character_set[i]) {
            if (!first) oss << ", ";
            oss << i;
            first = false;
        }
    }
    oss << "}";
    return oss.str();
}
