#pragma once

enum Solver {
    GUROBI_MIS,
    GUROBI_MDD,
    GUROBI_GRAPH,
    ENUMERATION,
};
constexpr Solver SOLVER = GUROBI_GRAPH;

constexpr int HEURISTIC_SOLUTION_DECREMENTER = 0;
