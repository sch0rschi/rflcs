#pragma once

#include "../instance.hpp"

void solve_gurobi_mis_ilp(instance &instance);
void solve_gurobi_mdd_ilp(instance &instance);
void solve_gurobi_graph_ilp(instance &instance);
void solve_gurobi_mdd_edges_ilp(instance &instance);
