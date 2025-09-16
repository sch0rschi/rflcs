#include "ilp_solvers.hpp"

#include <ranges>
#include <set>
#include <cmath>
#include <iostream>

#include "gurobi_c++.h"
#include "../config.hpp"
#include "absl/container/flat_hash_map.h"

void set_solution_from_graph(
    instance &instance,
    const std::set<rflcs_graph::match *> &matches,
    const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map);

std::set<rflcs_graph::match *> get_active_matches(const instance &instance);

absl::flat_hash_map<int, std::vector<rflcs_graph::match *> >
get_character_matches_map(const std::set<rflcs_graph::match *> &matches);

void set_objective_function(GRBModel &model,
                            const std::set<rflcs_graph::match *> &matches,
                            const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map);

void set_repetition_free_constraint(GRBModel &model,
                                    const std::set<rflcs_graph::match *> &matches,
                                    const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map);

void set_common_sub_sequence_constraint(GRBModel &model,
                                        const std::set<rflcs_graph::match *> &matches,
                                        const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map);

class SubsequenceLazyCallback final : public GRBCallback {
    const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map;

public:
    SubsequenceLazyCallback(
        const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map) : gurobi_variable_map(gurobi_variable_map) {
    }

protected:
    void callback() override {
        try {
            if (where == GRB_CB_MIPSOL) {
                auto solution_matches = std::vector<rflcs_graph::match *>();
                for (auto &[match, variable]: gurobi_variable_map) {
                    if ( getSolution(variable) > 0.5) {
                        solution_matches.emplace_back(match);
                    }
                }

                for (auto solution_match_1: solution_matches) {
                    for (auto solution_match_2: solution_matches) {
                        if (solution_match_1->extension.position_1 < solution_match_2->extension.position_1 &&
                            solution_match_1->extension.position_2 > solution_match_2->extension.position_2) {
                            GRBLinExpr expr = gurobi_variable_map.at(solution_match_1) + gurobi_variable_map.at(solution_match_2);
                            addLazy(expr <= 1.0);
                        }
                    }
                }
            }
        } catch (GRBException &e) {
            std::cerr << "Lazy callback error: " << e.getMessage() << std::endl;
        }
    }
};

void solve_gurobi_mis_ilp(instance &instance) {
    try {
        auto env = GRBEnv(true);

        env.start();
        env.set(GRB_DoubleParam_TimeLimit, constants::solver_timeout);
        env.set(GRB_IntParam_LogToConsole, 1);
        env.set(GRB_IntParam_Threads, 1);
        env.set(GRB_IntParam_MIPFocus, GRB_MIPFOCUS_BESTBOUND); // focus on upper bound
        env.set(GRB_IntParam_Presolve, GRB_PRESOLVE_AGGRESSIVE); // Aggressive presolve
        env.set(GRB_IntParam_ScaleFlag, 3);
        env.set(GRB_IntParam_LazyConstraints, 1);

        auto model = GRBModel(env);
        const auto matches = get_active_matches(instance);
        auto gurobi_variable_map = absl::flat_hash_map<rflcs_graph::match *, GRBVar>();

        SubsequenceLazyCallback cb(gurobi_variable_map);
        model.setCallback(&cb);
        for (const auto match: matches) {
            gurobi_variable_map[match] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
        }

        set_objective_function(model, matches, gurobi_variable_map);
        set_common_sub_sequence_constraint(model, matches, gurobi_variable_map);
        set_repetition_free_constraint(model, matches, gurobi_variable_map);
        model.optimize();

        const auto result_status = model.get(GRB_IntAttr_Status);
        instance.is_valid_solution = result_status == GRB_OPTIMAL || result_status == GRB_INFEASIBLE;
        if (model.get(GRB_IntAttr_SolCount) > 0) {
            temporaries::lower_bound = static_cast<int>(round(model.get(GRB_DoubleAttr_ObjVal)));
            temporaries::upper_bound = temporaries::lower_bound;
            set_solution_from_graph(instance, matches, gurobi_variable_map);
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

void set_common_sub_sequence_constraint(
    GRBModel &model,
    const std::set<rflcs_graph::match *> &matches,
    const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map) {
    for (const auto match1: matches) {
        for (const auto match2: matches) {
            if (std::abs(match1->upper_bound - match2->upper_bound) < temporaries::upper_bound - temporaries::lower_bound
                && std::abs(match1->extension.reversed->upper_bound - match2->extension.reversed->upper_bound) < temporaries::upper_bound - temporaries::lower_bound
                && match1->extension.position_1 < match2->extension.position_1
                && match1->extension.position_2 > match2->extension.position_2) {
                auto conflict = GRBLinExpr();
                conflict += gurobi_variable_map.at(match1);
                conflict += gurobi_variable_map.at(match2);
                model.addConstr(conflict, GRB_LESS_EQUAL, 1.0);
            }
        }
    }
}

void set_repetition_free_constraint(
    GRBModel &model,
    const std::set<rflcs_graph::match *> &matches,
    const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map) {
    for (const auto &[character, matches_with_character]: get_character_matches_map(matches)) {
        auto same_character_sum = GRBLinExpr();
        for (const auto same_character_match: matches_with_character) {
            same_character_sum += gurobi_variable_map.at(same_character_match);
        }
        model.addConstr(same_character_sum, GRB_LESS_EQUAL, 1.0);
    }
}

void set_objective_function(GRBModel &model,
                            const std::set<rflcs_graph::match *> &matches,
                            const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map) {
    auto objective = GRBLinExpr();
    for (const auto match: matches) {
        objective += gurobi_variable_map.at(match);
    }
    model.setObjective(objective, GRB_MAXIMIZE);
    model.addConstr(objective, GRB_GREATER_EQUAL, temporaries::lower_bound + 1);
}

absl::flat_hash_map<int, std::vector<rflcs_graph::match *> >
get_character_matches_map(const std::set<rflcs_graph::match *> &matches) {
    auto character_matches_map = absl::flat_hash_map<int, std::vector<rflcs_graph::match *> >();
    for (const auto match: matches) {
        if (!character_matches_map.contains(match->character)) {
            character_matches_map.insert({match->character, std::vector<rflcs_graph::match *>()});
        }
        character_matches_map[match->character].push_back(match);
    }
    return character_matches_map;
}

std::set<rflcs_graph::match *> get_active_matches(const instance &instance) {
    auto matches = std::set<rflcs_graph::match *>();
    for (auto &match: instance.graph->matches | std::ranges::views::drop(1) |
                      std::ranges::views::take(instance.graph->matches.size() - 2)) {
        if (match.extension.is_active) {
            matches.insert(&match);
        }
    }
    return matches;
}

void set_solution_from_graph(instance &instance,
                             const std::set<rflcs_graph::match *> &matches,
                             const absl::flat_hash_map<rflcs_graph::match *, GRBVar> &gurobi_variable_map) {
    auto matches_in_solution = std::vector<rflcs_graph::match *>();
    std::ranges::copy_if(matches, std::back_inserter(matches_in_solution),
                         [gurobi_variable_map](const rflcs_graph::match *match) {
                             return static_cast<int>(round(gurobi_variable_map.at(match).get(GRB_DoubleAttr_X))) ==
                                    1;
                         });

    std::ranges::sort(matches_in_solution, [](const rflcs_graph::match *m1, const rflcs_graph::match *m2) {
        return m1->extension.position_1 < m2->extension.position_1;
    });

    instance.solution.clear();
    for (const auto match_in_solution: matches_in_solution) {
        instance.solution.push_back(match_in_solution->character);
    }
}
