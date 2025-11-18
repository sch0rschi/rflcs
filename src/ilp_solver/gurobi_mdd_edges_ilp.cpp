#include "ilp_solvers.hpp"
#include "gurobi_c++.h"
#include "../constants.hpp"
#include "absl/container/flat_hash_set.h"

#include <ranges>
#include <cmath>


void set_solution_from_mdd_edges(instance &instance,
                                 const absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar> &
                                 gurobi_edges_map,
                                 const node *sink, const node *root);

absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar>
get_gurobi_edges_map(const instance &instance, GRBModel &model, node *sink);

void model_repetition_free_constraints(
    GRBModel &model,
    const node *sink,
    const node *root,
    const absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar> &gurobi_edges_map);

void model_mdd_traversal_constraints(const instance &instance,
                                     GRBModel &model,
                                     const node *sink,
                                     const node *root,
                                     const absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar>
                                     &gurobi_edges_map);

void model_lower_bound_levels_guidance(GRBModel &model,
                                       const absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar>
                                       &gurobi_edges_map);

void solve_gurobi_mdd_edges_ilp(instance &instance) {
    try {
        auto env = GRBEnv(true);

        env.start();
        env.set(GRB_DoubleParam_TimeLimit, constants::solver_timeout);
        env.set(GRB_IntParam_LogToConsole, 1);
        env.set(GRB_IntParam_Threads, 1);
        env.set(GRB_IntParam_MIPFocus, GRB_MIPFOCUS_BESTBOUND); // focus on upper bound
        env.set(GRB_IntParam_Presolve, GRB_PRESOLVE_AUTO); // Aggressive presolve
        env.set(GRB_IntParam_ScaleFlag, 3);

        auto model = GRBModel(env);

        const auto sink = std::make_unique<node>();
        const auto root = instance.mdd->levels.front()->nodes.front();
        auto objective = GRBLinExpr();
        auto gurobi_edges_map = get_gurobi_edges_map(instance, model, sink.get());
        for (auto const &[level_from_to, variable]: gurobi_edges_map) {
            if (level_from_to.first > 0) {
                objective += variable;
            }
        }

        model_repetition_free_constraints(model, sink.get(), root, gurobi_edges_map);
        model_mdd_traversal_constraints(instance, model, sink.get(), root, gurobi_edges_map);
        model_lower_bound_levels_guidance(model, gurobi_edges_map);

        model.setObjective(objective, GRB_MAXIMIZE);
        model.addConstr(objective, GRB_GREATER_EQUAL, temporaries::lower_bound + 1);

        model.optimize();

        const auto result_status = model.get(GRB_IntAttr_Status);
        instance.is_valid_solution = result_status == GRB_OPTIMAL || result_status == GRB_INFEASIBLE;
        if (model.get(GRB_IntAttr_SolCount) > 0) {
            temporaries::lower_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjVal)));
            temporaries::upper_bound = temporaries::lower_bound;
            set_solution_from_mdd_edges(instance, gurobi_edges_map, sink.get(), root);
        } else if (result_status == GRB_INFEASIBLE) {
            temporaries::upper_bound = temporaries::lower_bound;
        } else {
            temporaries::upper_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjBound)));
        }
    } catch
    (GRBException &e) {
        std::cout << "Error code = " << e.getErrorCode() << std::endl;
        std::cout << e.getMessage() << std::endl;
    }
}

absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar>
get_gurobi_edges_map(
    const instance &instance,
    GRBModel &model,
    node *sink) {
    auto gurobi_edges_map = absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar>();
    for (const auto &level: instance.mdd->levels) {
        for (const auto node: level->nodes) {
            for (const auto successor: node->edges_out) {
                gurobi_edges_map[{level->depth, {node, successor}}] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
            }
            if (level->depth > temporaries::lower_bound) {
                gurobi_edges_map[{level->depth, {node, sink}}] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
            }
        }
    }
    return gurobi_edges_map;
}

void model_repetition_free_constraints(
    GRBModel &model,
    const node *sink,
    const node *root,
    const absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar> &gurobi_edges_map) {
    auto character_incoming_edge_sums = std::vector<GRBLinExpr>(constants::alphabet_size);
    auto character_outgoing_edge_sums = std::vector<GRBLinExpr>(constants::alphabet_size);
    for (auto const &[level_from_to, variable]: gurobi_edges_map) {
        auto [from, to] = level_from_to.second;
        if (to != sink) {
            character_incoming_edge_sums.at(to->character) += variable;
        }
        if (from != root) {
            character_outgoing_edge_sums.at(from->character) += variable;
        }
    }
    for (auto const &character_outgoing_edge_sum: character_outgoing_edge_sums) {
        model.addConstr(character_outgoing_edge_sum <= 1);
    }
    for (auto const &character_incoming_edge_sum: character_incoming_edge_sums) {
        model.addConstr(character_incoming_edge_sum <= 1);
    }
}

void model_mdd_traversal_constraints(
    const instance &instance,
    GRBModel &model,
    const node *sink,
    const node *root,
    const absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar> &gurobi_edges_map) {
    auto node_incoming_edges_sums = absl::flat_hash_map<const node *, GRBLinExpr>();
    auto node_outgoing_edges_sums = absl::flat_hash_map<const node *, GRBLinExpr>();

    for (const auto &level: instance.mdd->levels) {
        for (const auto node: level->nodes) {
            node_incoming_edges_sums[node] = GRBLinExpr();
            node_outgoing_edges_sums[node] = GRBLinExpr();
        }
    }
    node_incoming_edges_sums[sink] = GRBLinExpr();

    for (auto const &[level_from_to, variable]: gurobi_edges_map) {
        auto [from, to] = level_from_to.second;
        node_outgoing_edges_sums.at(from) += variable;
        node_incoming_edges_sums.at(to) += variable;
    }

    for (const auto &level: instance.mdd->levels) {
        for (const auto node: level->nodes) {
            if (node == root) {
                model.addConstr(node_outgoing_edges_sums.at(node) == 1);
            } else if (node == sink) {
                model.addConstr(node_incoming_edges_sums.at(node) == 1);
            } else {
                model.addConstr(node_outgoing_edges_sums.at(node) == node_incoming_edges_sums.at(node));
            }
        }
    }
}

void model_lower_bound_levels_guidance(GRBModel &model,
                                       const absl::flat_hash_map<std::pair<int, std::pair<node *, node *> >, GRBVar>
                                       &gurobi_edges_map) {
    auto level_edges_sums = absl::flat_hash_map<int, GRBLinExpr>();
    for (int level = 0; level <= temporaries::lower_bound; ++level) {
        level_edges_sums[level] = GRBLinExpr();
    }
    for (auto const &[level_from_to, variable]: gurobi_edges_map) {
        if (level_from_to.first <= temporaries::lower_bound) {
            level_edges_sums[level_from_to.first] += variable;
        }
    }
    for (auto const &variable_sum: level_edges_sums | std::views::values) {
        model.addConstr(variable_sum >= 1);
    }
}

void set_solution_from_mdd_edges(instance &instance,
                                 const absl::flat_hash_map<
                                     std::pair<int, std::pair<node *, node *> >,
                                     GRBVar> &gurobi_edges_map,
                                 const node *sink,
                                 const node *root) {
    auto matches = absl::flat_hash_set<rflcs_graph::match *>();
    for (auto const &[depth_from_to, variable]: gurobi_edges_map) {
        if (static_cast<int>(round(variable.get(GRB_DoubleAttr_X))) == 1) {
            auto [from, to] = depth_from_to.second;
            if (to != sink) {
                matches.insert(static_cast<rflcs_graph::match *>(to->associated_match));
            }
            if (from != root) {
                matches.insert(static_cast<rflcs_graph::match *>(from->associated_match));
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
