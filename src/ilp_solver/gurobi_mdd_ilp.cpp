#include "ilp_solvers.hpp"

#include <ranges>
#include <cmath>

#include "gurobi_c++.h"
#include "../config.hpp"

#include <absl/container/flat_hash_map.h>

void set_solution_from_ilp(instance &instance, absl::flat_hash_map<node*, GRBVar>& gurobi_variable_map);

void solve_gurobi_mdd_ilp(instance &instance) {

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

        auto objective = GRBLinExpr();
        std::vector<GRBLinExpr> character_sums(constants::alphabet_size);
        const GRBLinExpr *previous_level_node_sum = nullptr;
        auto gurobi_variable_map = absl::flat_hash_map<node*, GRBVar>();

        for (const auto &level : instance.mdd->levels | std::views::drop(1)) {
            auto level_node_sum = GRBLinExpr();
            for(const auto node : level->nodes) {
                gurobi_variable_map[node] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
                character_sums.at(node->character) += gurobi_variable_map.at(node);
                level_node_sum += gurobi_variable_map.at(node);
                objective += gurobi_variable_map.at(node);
                if(level->depth > 1) {
                    auto preds = GRBLinExpr();
                    for(const auto pred : node->edges_in) {
                        preds += gurobi_variable_map.at(pred);
                    }
                    model.addConstr(gurobi_variable_map.at(node), GRB_LESS_EQUAL, preds);
                }
            }
            if (level->depth <= temporaries::lower_bound + 1) {
                model.addConstr(level_node_sum, GRB_EQUAL, 1);
            } else {
                model.addConstr(level_node_sum, GRB_LESS_EQUAL, 1);
            }
            if(previous_level_node_sum != nullptr) {
                model.addConstr(level_node_sum, GRB_LESS_EQUAL, *previous_level_node_sum);
            }
            previous_level_node_sum = &level_node_sum;
        }

        for (const auto& character_sum : character_sums) {
            model.addConstr(character_sum, GRB_LESS_EQUAL, 1);
        }
        model.addConstr(objective, GRB_GREATER_EQUAL, temporaries::lower_bound + 1);
        model.setObjective(objective, GRB_MAXIMIZE);
        model.optimize();

        const auto result_status = model.get(GRB_IntAttr_Status);
        instance.is_valid_solution = result_status == GRB_OPTIMAL || result_status == GRB_INFEASIBLE;
        if (model.get(GRB_IntAttr_SolCount) > 0) {
            temporaries::lower_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjVal)));
            temporaries::upper_bound = temporaries::lower_bound;
            set_solution_from_ilp(instance, gurobi_variable_map);
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

void set_solution_from_ilp(instance &instance, absl::flat_hash_map<node*, GRBVar>& gurobi_variable_map) {
    instance.solution.clear();
    for (const auto &level : instance.mdd->levels | std::views::drop(1)) {
        for(const auto node : level->nodes) {
            if (static_cast<int>(round(gurobi_variable_map.at(node).get(GRB_DoubleAttr_X))) == 1) {
                instance.solution.push_back(node->character);
            }
        }
    }
    if (!instance.is_solving_forward) {
        std::ranges::reverse(instance.solution);
    }
}
