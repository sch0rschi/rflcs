#include "ilp_solvers.hpp"
#include "gurobi_c++.h"
#include "../constants.hpp"
#include "../mdd_graph_pruning.hpp"
#include "absl/container/flat_hash_set.h"

#include <cmath>

#include "../graph/match_loop_utils.hpp"

absl::flat_hash_map<std::pair<rflcs_graph::match *, rflcs_graph::match *>, GRBVar> get_gurobi_edges_map(
    GRBModel &model,
    const std::vector<rflcs_graph::match *> &matches,
    rflcs_graph::match *sink);

std::vector<rflcs_graph::match *> get_active_matches(const instance &instance);

void set_solution_from_edges(instance &instance,
                             const absl::flat_hash_map<std::pair<rflcs_graph::match *, rflcs_graph::match *>, GRBVar> &
                             gurobi_edges_map,
                             const rflcs_graph::match *sink,
                             const rflcs_graph::match *root);

void solve_gurobi_graph_edges_ilp(instance &instance) {
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
        update_graph_by_mdd(instance);
        auto matches = get_active_matches(instance);
        auto root = matches.front();
        std::cout << root << std::endl;
        const auto sink = std::make_unique<rflcs_graph::match>();

        auto gurobi_edges_map = get_gurobi_edges_map(model, matches, sink.get());

        auto objective = GRBLinExpr();
        auto character_edge_sums = std::vector<GRBLinExpr>(constants::alphabet_size);
        auto root_edges_sum = GRBLinExpr();
        auto sink_edges_sum = GRBLinExpr();

        for (auto const &[from_to, gurobi_variable]: gurobi_edges_map) {

            auto from = from_to.first;
            auto to = from_to.second;
            if (to != sink.get()) {
                objective += gurobi_variable;
            }
            if (to == sink.get()) {
                sink_edges_sum += gurobi_variable;
            } else {
                character_edge_sums.at(to->character) += gurobi_variable;
            }
            if (from == root) {
                root_edges_sum += gurobi_variable;
            } else {
                character_edge_sums.at(from->character) += gurobi_variable;
            }
        }

        for (auto const &character_edge_sum: character_edge_sums) {
            model.addConstr(character_edge_sum <= 2);
        }
        model.addConstr(root_edges_sum == 1);
        model.addConstr(sink_edges_sum == 1);

        auto incoming_edges_sum_for_match = absl::flat_hash_map<rflcs_graph::match *, GRBLinExpr>();
        auto outgoing_edges_sum_for_match = absl::flat_hash_map<rflcs_graph::match *, GRBLinExpr>();
        for (auto match: matches) {
            incoming_edges_sum_for_match[match] = GRBLinExpr();
            outgoing_edges_sum_for_match[match] = GRBLinExpr();
        }
        for (auto const &[from_to, variable]: gurobi_edges_map) {
            if (auto to = from_to.second; to != sink.get()) {
                incoming_edges_sum_for_match.at(to) += variable;
            }
            if (auto from = from_to.first; from != root) {
                outgoing_edges_sum_for_match.at(from) += variable;
            }
        }
        for (auto match: matches) {
            if (match != root && match != sink.get()) {
                model.addConstr(outgoing_edges_sum_for_match.at(match) == incoming_edges_sum_for_match.at(match));
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

absl::flat_hash_map<std::pair<rflcs_graph::match *, rflcs_graph::match *>, GRBVar> get_gurobi_edges_map(
    GRBModel &model,
    const std::vector<rflcs_graph::match *> &matches,
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
