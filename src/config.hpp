#pragma once

#include <string_view>

enum Solver {
    GUROBI_GRAPH,
    GUROBI_MDD,
    ENUMERATION,
    MULTI,
};
constexpr Solver SOLVER = MULTI;

constexpr std::string_view default_path = "../RFLCS_instances/type1/512_8reps.0";
constexpr long MDD_TIMEOUT_IN_SECONDS = 1800;
constexpr int SOLVER_TIMEOUT_IN_SECONDS = 1800;
constexpr int HEURISTIC_SOLUTION_DECREMENTER = 0;
