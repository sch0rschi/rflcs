#include "ilp_solvers.hpp"

#include <ranges>
#include <cmath>

#include "gurobi_c++.h"
#include "../config.hpp"

#include <absl/container/flat_hash_map.h>

#include "absl/container/flat_hash_set.h"

void remove_duplicates(std::vector<rflcs_graph::match *> &matches);

void remove_dominated(std::vector<rflcs_graph::match *> &matches);

absl::flat_hash_set<rflcs_graph::match *> get_matches(const instance &instance);

absl::flat_hash_map<rflcs_graph::match *, GRBVar> create_gurobi_variables(
    GRBModel &model,
    const absl::flat_hash_set<rflcs_graph::match *> &matches);

void update_graph_by_mdd(const instance &instance, absl::flat_hash_set<rflcs_graph::match *> &matches);

void solve_gurobi_graph_ilp(instance &instance) {
    try {
        auto env = GRBEnv(true);

        env.start();
        env.set(GRB_DoubleParam_TimeLimit, SOLVER_TIMEOUT_IN_SECONDS);
        env.set(GRB_IntParam_LogToConsole, 1);
        env.set(GRB_IntParam_Threads, 1);
        env.set(GRB_IntParam_MIPFocus, GRB_MIPFOCUS_BESTBOUND); // focus on upper bound
        env.set(GRB_IntParam_Presolve, GRB_PRESOLVE_AGGRESSIVE); // Aggressive presolve
        env.set(GRB_IntParam_ScaleFlag, 3);

        auto model = GRBModel(env);

        absl::flat_hash_set<rflcs_graph::match *> matches = get_matches(instance);
        absl::flat_hash_map<rflcs_graph::match *, GRBVar> gurobi_variable_map = create_gurobi_variables(model, matches);
        update_graph_by_mdd(instance, matches);

        std::vector<GRBLinExpr> character_count(constants::alphabet_size);
        for (auto const &[match, gurobi_variable]: gurobi_variable_map) {
            if (match->character < constants::alphabet_size) {
                character_count[match->character] += gurobi_variable;
            }
        }
        for (int character = 0; character < constants::alphabet_size; character++) {
            model.addConstr(character_count[character], GRB_LESS_EQUAL, 1);
        }

        auto objective = GRBLinExpr();
        for (auto const &[match, gurobi_variable]: gurobi_variable_map) {
            if (match->character < constants::alphabet_size) {
                objective += gurobi_variable;
            }
        }
        model.setObjective(objective, GRB_MAXIMIZE);
        model.addConstr(objective, GRB_GREATER_EQUAL, temporaries::lower_bound + 1);

        for (auto match: matches) {
            for (auto succ_match_1: match->extension.succ_matches) {
                for (auto succ_match_2: match->extension.succ_matches) {
                    if (succ_match_1->extension.position_1 < succ_match_2->extension.position_1
                        && succ_match_1->extension.position_2 > succ_match_2->extension.position_2) {
                        model.addConstr(
                            gurobi_variable_map.at(succ_match_1) + gurobi_variable_map.at(succ_match_2) <= 1);
                    }
                }
            }

            if (match->character < constants::alphabet_size) {
                auto predecessor_sum = GRBLinExpr();
                for (auto pred_match: match->extension.pred_matches) {
                    predecessor_sum += gurobi_variable_map.at(pred_match);
                }
                model.addConstr(gurobi_variable_map.at(match), GRB_LESS_EQUAL, predecessor_sum);
            }

            if (match->extension.reversed->upper_bound <= temporaries::lower_bound) {
                auto successor_sum = GRBLinExpr();
                for (auto succ_match: match->extension.succ_matches) {
                    successor_sum += gurobi_variable_map.at(succ_match);
                }
                model.addConstr(successor_sum, GRB_GREATER_EQUAL, gurobi_variable_map.at(match));
            }
        }

        model.optimize();

        const auto result_status = model.get(GRB_IntAttr_Status);
        instance.is_valid_solution = result_status == GRB_OPTIMAL || result_status == GRB_INFEASIBLE;
        if (model.get(GRB_IntAttr_SolCount) > 0) {
            temporaries::lower_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjVal)));
            temporaries::upper_bound = temporaries::lower_bound;
            // TODO: set solution
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
            matches.insert(node->match);
        }
    }
    return matches;
}

absl::flat_hash_map<rflcs_graph::match *, GRBVar> create_gurobi_variables(
    GRBModel &model, const absl::flat_hash_set<rflcs_graph::match *> &matches) {
    auto gurobi_variable_map = absl::flat_hash_map<rflcs_graph::match *, GRBVar>();
    for (const auto &match: matches) {
        gurobi_variable_map[match] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
    }
    return gurobi_variable_map;
}

void update_graph_by_mdd(const instance &instance, absl::flat_hash_set<rflcs_graph::match *> &matches) {
    for (const auto &match: matches) {
        match->dom_succ_matches.clear();
        match->extension.succ_matches.clear();
    }

    for (const auto &level: instance.mdd->levels) {
        for (const auto &node: level->nodes) {
            const auto match = node->match;
            for (const auto succ_node: node->edges_out) {
                auto succ_match = succ_node->match;
                match->dom_succ_matches.push_back(succ_match);
                match->extension.succ_matches.push_back(succ_match);
            }
        }
    }

    for (auto &match: matches) {
        remove_duplicates(match->dom_succ_matches);
        remove_dominated(match->dom_succ_matches);
        remove_duplicates(match->extension.succ_matches);
    }

    for (auto &match: matches) {
        for (const auto dom_succ_match: match->dom_succ_matches) {
            dom_succ_match->extension.dom_pred_matches.push_back(match);
        }
        for (const auto succ_match: match->extension.succ_matches) {
            succ_match->extension.pred_matches.push_back(match);
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
        return m1->extension.position_1 < m2->extension.position_1;
    });
    const auto matches_copy = matches;
    matches.clear();
    int max_position_2 = INT_MAX;
    for (auto match: matches_copy) {
        if (match->extension.position_2 < max_position_2) {
            matches.push_back(match);
            max_position_2 = match->extension.position_2;
        }
    }
}
