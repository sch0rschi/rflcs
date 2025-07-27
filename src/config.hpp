#pragma once

#include <string_view>

#ifndef CHARACTER_SET_SIZE
#define CHARACTER_SET_SIZE 512
#endif

enum Solver {
    GUROBI_GRAPH,
    GUROBI_MDD,
    ENUMERATION,
    MULTI,
};

constexpr std::string_view default_path = "../RFLCS_instances/type1/512_8reps.24";
constexpr long MDD_TIMEOUT_IN_SECONDS = 1800;
constexpr int SOLVER_TIMEOUT_IN_SECONDS = 1800;
constexpr bool IS_WRITING_DOT_FILE = false;
constexpr bool IS_WRITING_TIME_SERIES = false;
constexpr Solver SOLVER = MULTI;
constexpr int HEURISTIC_SOLUTION_DECREMENTER = 0;
