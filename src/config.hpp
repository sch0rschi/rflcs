#pragma once

#include <string_view>

enum Solver {
    GUROBI_MIS,
    GUROBI_MDD,
    GUROBI_GRAPH,
    ENUMERATION,
};
constexpr Solver SOLVER = GUROBI_GRAPH;

constexpr std::string_view default_path = "../RFLCS_instances/type1/512_8reps.24";
constexpr long MDD_TIMEOUT_IN_SECONDS = 1800;
constexpr int SOLVER_TIMEOUT_IN_SECONDS = 1800;
constexpr int HEURISTIC_SOLUTION_DECREMENTER = 0;
