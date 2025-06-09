#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string_view>

enum Solver {
    GUROBI_GRAPH,
    GUROBI_MDD,
    ENUMERATION,
    MULTI,
};

constexpr std::string_view default_path = "../RFLCS_instances/type1/8_3reps.0";
constexpr long MDD_TIMEOUT_IN_SECONDS = 1800;
constexpr int SOLVER_TIMEOUT_IN_SECONDS = 1800;
constexpr bool IS_WRITING_DOT_FILE = true;
constexpr bool IS_WRITING_TIME_SERIES = false;
constexpr Solver SOLVER = MULTI;
constexpr int HEURISTIC_SOLUTION_DECREMENTER = 2;

#endif
