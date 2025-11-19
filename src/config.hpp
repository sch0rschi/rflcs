#pragma once

#include <string_view>

enum Ilp_Solver {
    GUROBI_GRAPH_MATCH_MIS,
    GUROBI_GRAPH_EDGES,
    GUROBI_GRAPH_DOMINATING_MATCHES, // TODO: set solution
    GUROBI_MDD_NODES,
    GUROBI_MDD_EDGES,
};
constexpr Ilp_Solver SOLVER = GUROBI_GRAPH_EDGES;

constexpr int HEURISTIC_SOLUTION_DECREMENTER = 0;

constexpr int REDUCTION_TIMEOUT = 1800;
constexpr int SOLVER_TIMEOUT = 1800;
constexpr std::string_view DEFAULT_INPUT_FILE = "RFLCS_instances/type1/128_8reps.24";