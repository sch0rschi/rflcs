#include "ilp_solvers.hpp"

#include <ranges>
#include <cmath>

#include "gurobi_c++.h"
#include "../constants.hpp"

#include <absl/container/flat_hash_map.h>

#include "absl/container/flat_hash_set.h"

absl::flat_hash_map<std::pair<rflcs_graph::match *, rflcs_graph::match *>, GRBVar> get_gurobi_edges_map(
    GRBModel &model,
    const absl::flat_hash_set<rflcs_graph::match *> &matches,
    rflcs_graph::match *sink);

void remove_duplicates(std::vector<rflcs_graph::match *> &matches);

void remove_dominated(std::vector<rflcs_graph::match *> &matches);

absl::flat_hash_set<rflcs_graph::match *> get_matches(const instance &instance);

void update_graph_by_mdd(const instance &instance, absl::flat_hash_set<rflcs_graph::match *> &matches);

void set_solution_from_edges(instance &instance,
                             const absl::flat_hash_map<std::pair<rflcs_graph::match *, rflcs_graph::match *>, GRBVar> &
                             gurobi_edges_map,
                             const rflcs_graph::match *sink,
                             const rflcs_graph::match *root);

void solve_gurobi_graph_ilp(instance &instance) {
    try {
        auto env = GRBEnv(true);

        env.start();
        env.set(GRB_DoubleParam_TimeLimit, constants::solver_timeout);
        env.set(GRB_IntParam_LogToConsole, 1);
        env.set(GRB_IntParam_Threads, 1);
        env.set(GRB_IntParam_MIPFocus, GRB_MIPFOCUS_BESTBOUND); // focus on upper bound
        env.set(GRB_IntParam_Presolve, GRB_PRESOLVE_AGGRESSIVE); // Aggressive presolve
        env.set(GRB_IntParam_ScaleFlag, 3);

        auto model = GRBModel(env);

        auto matches = get_matches(instance);
        rflcs_graph::match *root = nullptr;
        for (auto match: matches) {
            if (match->character >= constants::alphabet_size) {
                root = match;
            }
        }
        const auto sink = std::make_unique<rflcs_graph::match>();

        update_graph_by_mdd(instance, matches);
        auto gurobi_edges_map = get_gurobi_edges_map(model, matches, sink.get());

        auto objective = GRBLinExpr();
        auto incoming_edge_sum_for_character = std::vector<GRBLinExpr>(constants::alphabet_size);
        auto outgoing_edge_sum_for_character = std::vector<GRBLinExpr>(constants::alphabet_size);
        auto outgoing_edges_sum_root = GRBLinExpr();
        auto incoming_edges_sum_sink = GRBLinExpr();
        for (auto const &[from_to, gurobi_variable]: gurobi_edges_map) {
            auto from = from_to.first;
            auto to = from_to.second;
            if (to != sink.get()) {
                objective += gurobi_variable;
            }
            if (to == sink.get()) {
                incoming_edges_sum_sink += gurobi_variable;
            } else {
                incoming_edge_sum_for_character.at(to->character) += gurobi_variable;
            }
            if (from == root) {
                outgoing_edges_sum_root += gurobi_variable;
            } else {
                outgoing_edge_sum_for_character.at(from->character) += gurobi_variable;
            }
        }
        for (auto const &outgoing_edge_sum: outgoing_edge_sum_for_character) {
            model.addConstr(outgoing_edge_sum <= 1);
        }
        for (auto const &incoming_edge_sum: incoming_edge_sum_for_character) {
            model.addConstr(incoming_edge_sum <= 1);
        }
        model.addConstr(outgoing_edges_sum_root == 1);
        model.addConstr(incoming_edges_sum_sink == 1);

        auto incoming_edges_sum_for_match = absl::flat_hash_map<rflcs_graph::match *, GRBLinExpr>();
        auto outgoing_edges_sum_for_match = absl::flat_hash_map<rflcs_graph::match *, GRBLinExpr>();
        for (auto match: matches) {
            incoming_edges_sum_for_match[match] = GRBLinExpr();
            outgoing_edges_sum_for_match[match] = GRBLinExpr();
        }
        for (auto const &[from_to, variable]: gurobi_edges_map) {
            auto from = from_to.first;
            auto to = from_to.second;
            if (to != sink.get()) {
                incoming_edges_sum_for_match.at(to) += variable;
            }
            if (from != root) {
                outgoing_edges_sum_for_match.at(from) += variable;
            }
        }
        for (auto match: matches) {
            if (match != root && match != sink.get()) {
                model.addConstr(outgoing_edges_sum_for_match.at(match) <= incoming_edges_sum_for_match.at(match));
            }
        }

        model.setObjective(objective, GRB_MAXIMIZE);
        model.addConstr(objective, GRB_GREATER_EQUAL, temporaries::lower_bound + 1);

        model.optimize();

        const auto result_status = model.get(GRB_IntAttr_Status);
        instance.is_valid_solution = result_status == GRB_OPTIMAL || result_status == GRB_INFEASIBLE;
        if (model.get(GRB_IntAttr_SolCount) > 0) {
            temporaries::lower_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjVal)));
            temporaries::upper_bound = temporaries::lower_bound;
            set_solution_from_edges(instance, gurobi_edges_map, sink.get(), root);
        } else if (result_status == GRB_INFEASIBLE) {
            temporaries::upper_bound = temporaries::lower_bound;
        } else {
            temporaries::upper_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjBound)));
        }
    } catch (GRBException &e) {
        std::cout << "Error code = " << e.getErrorCode() << std::endl;
        std::cout << e.getMessage() << std::endl;
    }
}

absl::flat_hash_set<rflcs_graph::match *> get_matches(const instance &instance) {
    auto matches = absl::flat_hash_set<rflcs_graph::match *>();
    for (const auto &level: instance.mdd->levels) {
        for (const auto node: level->nodes) {
            matches.insert(static_cast<rflcs_graph::match *>(node->associated_match));
        }
    }
    return matches;
}

absl::flat_hash_map<std::pair<rflcs_graph::match *, rflcs_graph::match *>, GRBVar> get_gurobi_edges_map(
    GRBModel &model,
    const absl::flat_hash_set<rflcs_graph::match *> &matches,
    rflcs_graph::match *sink) {
    auto gurobi_edges_map = absl::flat_hash_map<std::pair<rflcs_graph::match *, rflcs_graph::match *>, GRBVar>();
    for (auto match: matches) {
        for (auto succ_match: match->extension->succ_matches) {
            gurobi_edges_map[std::make_pair(match, succ_match)] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
        }
        if (match->reversed->upper_bound > temporaries::lower_bound) {
            gurobi_edges_map[std::make_pair(match, sink)] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
        }
    }
    return gurobi_edges_map;
}

void update_graph_by_mdd(const instance &instance, absl::flat_hash_set<rflcs_graph::match *> &matches) {
    for (const auto &match: matches) {
        match->dom_succ_matches.clear();
        match->extension->succ_matches.clear();
    }

    for (const auto &level: instance.mdd->levels) {
        for (const auto &node: level->nodes) {
            const auto match = static_cast<rflcs_graph::match *>(node->associated_match);
            for (const auto succ_node: node->edges_out) {
                auto *succ_match = static_cast<rflcs_graph::match *>(succ_node->associated_match);
                match->dom_succ_matches.push_back(succ_match);
                match->extension->succ_matches.push_back(succ_match);
            }
        }
    }

    for (auto &match: matches) {
        remove_duplicates(match->dom_succ_matches);
        remove_dominated(match->dom_succ_matches);
        remove_duplicates(match->extension->succ_matches);
    }

    for (auto &match: matches) {
        for (const auto dom_succ_match: match->dom_succ_matches) {
            dom_succ_match->extension->dom_pred_matches.push_back(match);
        }
        for (const auto succ_match: match->extension->succ_matches) {
            succ_match->extension->pred_matches.push_back(match);
        }
    }
}

void remove_duplicates(std::vector<rflcs_graph::match *> &matches) {
    std::ranges::sort(matches);
    const auto [first, last] = std::ranges::unique(matches);
    matches.erase(first, last);
}

void remove_dominated(std::vector<rflcs_graph::match *> &matches) {
    std::ranges::sort(matches, [](const rflcs_graph::match *m1, const rflcs_graph::match *m2) {
        return m1->extension->position_1 < m2->extension->position_1;
    });
    const auto matches_copy = matches;
    matches.clear();
    int max_position_2 = std::numeric_limits<int>::max();
    for (auto match: matches_copy) {
        if (match->extension->position_2 < max_position_2) {
            matches.push_back(match);
            max_position_2 = match->extension->position_2;
        }
    }
}

void set_solution_from_edges(instance &instance,
                             const absl::flat_hash_map<
                                 std::pair<rflcs_graph::match *, rflcs_graph::match *>,
                                 GRBVar>
                             &gurobi_edges_map,
                             const rflcs_graph::match *sink,
                             const rflcs_graph::match *root) {
    auto matches = absl::flat_hash_set<rflcs_graph::match *>();
    for (auto const &[from_to, variable]: gurobi_edges_map) {
        if (static_cast<int>(round(variable.get(GRB_DoubleAttr_X))) == 1) {
            if (const auto to = from_to.second; to != sink) {
                matches.insert(to);
            }
            if (const auto from = from_to.first; from != root) {
                matches.insert(from);
            }
        }
    }
    std::vector matches_in_solution(matches.begin(), matches.end());
    std::ranges::sort(matches_in_solution, [](const rflcs_graph::match *m1, const rflcs_graph::match *m2) {
        return m1->extension->position_1 < m2->extension->position_1;
    });

    instance.solution.clear();
    for (const auto match_in_solution: matches_in_solution) {
        instance.solution.push_back(match_in_solution->character);
    }
    if (!instance.is_solving_forward) {
        std::ranges::reverse(instance.solution);
    }
}
