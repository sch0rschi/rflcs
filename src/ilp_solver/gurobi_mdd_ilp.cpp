#include "ilp_solvers.hpp"

#include <ranges>
#include <cmath>

#include "gurobi_c++.h"
#include "../config.hpp"

void set_solution_from_ilp(instance &instance);

void solve_gurobi_mdd_ilp(instance &instance) {

    try {
        auto env = GRBEnv(true);

        env.start();
        env.set(GRB_DoubleParam_TimeLimit, SOLVER_TIMEOUT_IN_SECONDS);
        env.set(GRB_IntParam_LogToConsole, 1);
        env.set(GRB_IntParam_Threads, 1);
        env.set(GRB_IntParam_MIPFocus, GRB_MIPFOCUS_BESTBOUND); // focus on upper bound
        // TODO: benchmark
        env.set(GRB_IntParam_Presolve, GRB_PRESOLVE_AGGRESSIVE); // Aggressive presolve
        // TODO: benchmark
        //env.set(GRB_IntParam_Cuts, 0);
        // TODO: benchmark
        env.set(GRB_IntParam_ScaleFlag, 3);
//        env.set(GRB_IntParam_VarBranch, GRB_VARBRANCH_MAX_INFEAS);

        auto model = GRBModel(env);

        auto objective = GRBLinExpr();
        std::vector<GRBLinExpr> character_sums(instance.alphabet_size);
        GRBLinExpr *previous_level_node_sum = nullptr;
        for (const auto &level : *instance.mdd->levels | std::views::drop(1)) {
            GRBLinExpr level_node_sum = GRBLinExpr();
            for(const auto node : *level->nodes) {
                node->gurobi_variable = model.addVar(0.0, 1.0, 0, GRB_BINARY);
                character_sums.at(node->match->character) += node->gurobi_variable;
                level_node_sum += node->gurobi_variable;
                objective += node->gurobi_variable;
                if(level->depth > 1) {
                    auto preds = GRBLinExpr();
                    for(auto pred : node->arcs_in) {
                        preds += pred->gurobi_variable;
                    }
                    model.addConstr(node->gurobi_variable, GRB_LESS_EQUAL, preds);
                }
            }
            if (level->depth <= instance.lower_bound + 1) {
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
        model.addConstr(objective, GRB_GREATER_EQUAL, instance.lower_bound + 1);
        model.setObjective(objective, GRB_MAXIMIZE);
        model.optimize();

        auto result_status = model.get(GRB_IntAttr_Status);
        instance.is_valid_solution = result_status == GRB_OPTIMAL || result_status == GRB_INFEASIBLE;
        if (model.get(GRB_IntAttr_SolCount) > 0) {
            instance.lower_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjVal)));
            instance.mdd_ilp_upper_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjBound)));
            set_solution_from_ilp(instance);
        } else if (result_status == GRB_INFEASIBLE) {
            instance.mdd_ilp_upper_bound = instance.lower_bound;
        } else {
            instance.mdd_ilp_upper_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjBound)));
        }
    } catch (GRBException &e) {
        std::cout << "Error code = " << e.getErrorCode() << std::endl;
        std::cout << e.getMessage() << std::endl;
    }
}

void set_solution_from_ilp(instance &instance) {
    if constexpr (SOLVER == GUROBI_MDD) {
        instance.solution.clear();
        for (const auto &level : *instance.mdd->levels | std::views::drop(1)) {
            for(const auto node : *level->nodes) {
                if (static_cast<int>(round(node->gurobi_variable.get(GRB_DoubleAttr_X))) == 1) {
                    instance.solution.push_back(node->match->character);
                }
            }
        }
        if (!instance.is_solving_forward) {
            std::ranges::reverse(instance.solution);
        }
    }
}
