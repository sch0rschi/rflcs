#include "ilp_solvers.hpp"
#include "match_utils.hpp"
#include "../mdd_graph_pruning.hpp"

#include <gurobi_c++.h>

void solve_gurobi_graph_dominating_matches_ilp(instance &instance) {
    try {
        auto env = GRBEnv(true);

        env.start();
        env.set(GRB_DoubleParam_TimeLimit, constants::solver_timeout);
        env.set(GRB_IntParam_LogToConsole, 1);
        env.set(GRB_IntParam_Threads, 1);
        env.set(GRB_IntParam_MIPFocus, GRB_MIPFOCUS_BESTBOUND); // focus on upper bound
        env.set(GRB_IntParam_Presolve, GRB_PRESOLVE_AGGRESSIVE); // Aggressive presolve

        auto model = GRBModel(env);

        update_graph_by_mdd(instance);
        auto active_matches = get_active_matches(instance);
        auto root = active_matches.front();
        active_matches.erase(active_matches.begin());
        auto objective = GRBLinExpr();

        auto gurobi_match_variable_map = absl::flat_hash_map<rflcs_graph::match*, GRBVar>();
        auto gurobi_character_variable_map = absl::flat_hash_map<Character, GRBVar>();
        auto gurobi_character_sums_map = absl::flat_hash_map<Character, GRBLinExpr>();
        auto match_predecessors_map = absl::flat_hash_map<rflcs_graph::match*, GRBLinExpr>();

        for (auto match : active_matches) {
            gurobi_match_variable_map[match] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
        }
        for (Character character = 0; character < constants::alphabet_size; character++) {
            gurobi_character_variable_map[character] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
            gurobi_character_sums_map[character] = GRBLinExpr();
        }

        for (auto match : active_matches) {
            gurobi_character_sums_map[match->character] += gurobi_match_variable_map[match];
            if (!match->dom_succ_matches.empty()) {
                auto dominating_successors = GRBLinExpr();
                for (auto successor : match->dom_succ_matches) {
                    dominating_successors += gurobi_match_variable_map[successor];
                    match_predecessors_map[successor] += gurobi_match_variable_map[match];
                }
                model.addConstr(dominating_successors <= 1);
                model.addConstr(dominating_successors >= gurobi_match_variable_map[match]);
            }

        }
        for (Character character = 0; character < constants::alphabet_size; character++) {
            model.addConstr(gurobi_character_sums_map[character] >= gurobi_character_variable_map[character]);
            objective += gurobi_character_variable_map[character];
        }

        for (auto match : active_matches) {
            if (match_predecessors_map.contains(match)) {
                model.addConstr(match_predecessors_map[match] >= gurobi_match_variable_map[match]);
            }
        }

        auto starting_matches = GRBLinExpr();
        for (auto starting_match : root->dom_succ_matches) {
            starting_matches += gurobi_match_variable_map[starting_match];
        }
        model.addConstr(starting_matches == 1);

        model.setObjective(objective, GRB_MAXIMIZE);

        model.addConstr(objective, GRB_GREATER_EQUAL, temporaries::lower_bound + 1);

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
