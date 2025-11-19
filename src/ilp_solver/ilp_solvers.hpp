#pragma once

#include "../instance.hpp"

void solve_gurobi_graph_match_mis_ilp(instance &instance);

void solve_gurobi_graph_edges_ilp(instance &instance);

void solve_gurobi_graph_dominating_matches_ilp(instance &instance);

void solve_gurobi_mdd_nodes_ilp(instance &instance);

void solve_gurobi_mdd_edges_ilp(instance &instance);
