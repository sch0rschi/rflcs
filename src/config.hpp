#pragma once

enum Ilp_Solver {
    GUROBI_MIS,
    GUROBI_MDD,
    GUROBI_GRAPH,
    GUROBI_MDD_EDGES,
};
constexpr Ilp_Solver SOLVER = GUROBI_GRAPH;

constexpr int HEURISTIC_SOLUTION_DECREMENTER = 0;
