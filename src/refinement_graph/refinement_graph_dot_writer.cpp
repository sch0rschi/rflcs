#include "header/refinement_graph_dot_writer.hpp"

std::string bitset_to_index_set(const boost::dynamic_bitset<> &character_set);

void write_refinement_graph(const instance &instance, const std::string &filename) {
    using Graph = boost::adjacency_list<
        boost::vecS,
        boost::vecS,
        boost::directedS,
        boost::property<boost::vertex_name_t, std::string> >;

    Graph graph;

    absl::flat_hash_map<refinement_node*, Vertex> vertex_map;

    for (auto const match: instance.active_refinement_match_pointers | std::views::reverse) {
        for (auto const node: match->refinement_nodes) {
            vertex_map[node] = boost::add_vertex(
                "(" + std::to_string(node->refinement_match->position_1) + ", "
                + std::to_string(node->refinement_match->position_2) + "), "
                + std::to_string(node->refinement_match->character) + "\n"
                + bitset_to_index_set(node->characters_on_paths_to_root) + " "
                + bitset_to_index_set(node->characters_on_paths_to_some_sink) + "\n"
                + bitset_to_index_set(node->characters_on_all_paths_to_root) + " "
                + bitset_to_index_set(node->characters_on_all_paths_to_lower_bound_length) + "\n"
                + std::to_string(node->upper_bound) + " "
                + std::to_string(node->upper_bound_up) + " "
                + std::to_string(node->upper_bound_down),
                graph);
            for (const auto succ: node->successors) {
                boost::add_edge(vertex_map[node], vertex_map[succ], graph);
            }
        }
    }

    for (auto const match: instance.active_refinement_match_pointers) {
        for (auto const node: match->refinement_nodes) {
            for (const auto predecessor: node->predecessors) {
                boost::add_edge(vertex_map[predecessor], vertex_map[node], graph);
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
