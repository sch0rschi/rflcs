#include "temporaries.hpp"
#include "graph/header/graph_creation.hpp"
#include "heuristic.hpp"
#include "instance.hpp"
#include "reduction_orchestration.hpp"
#include "result_writer.hpp"
#include "input_processing.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "solver/sequence_enumeration_solver.hpp"
#ifdef ILP_FEATURE
#include "ilp_solver/ilp_solvers.hpp"
#endif
#include "absl/container/flat_hash_set.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <sys/resource.h>


void initialize_temporaries();

void heuristic(instance &instance);

void print_heuristic_stats(const instance &instance);

void reduction(instance &instance);

void solve(instance &instance);

void check_solution(instance &instance);

void print_result_stats(const instance &instance);

int main(const int argc, char **argv) {
    try {
        instance instance;

        if (const auto cla_processing_status_code = process_command_line_arguments(argc, argv, instance);
            cla_processing_status_code != SUCCESS) {
            return 1;
        }

        if (const auto ipf_processing_status_code = process_input(instance);
            ipf_processing_status_code != SUCCESS) {
            return 1;
        }

        initialize_temporaries();

        instance.start = std::chrono::system_clock::now();
        create_graph(instance);

        heuristic(instance);
        instance.heuristic_solution_length = temporaries::lower_bound;
        instance.heuristic_end = std::chrono::system_clock::now();
        print_heuristic_stats(instance);

        reduction(instance);
        instance.reduction_end = std::chrono::system_clock::now();
        instance.reduction_upper_bound = temporaries::upper_bound;

        solve(instance);
        instance.end = std::chrono::system_clock::now();
        temporaries::upper_bound = std::max(temporaries::upper_bound, temporaries::lower_bound);
        check_solution(instance);

        write_result_file(instance);
        print_result_stats(instance);

        return 0;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

void initialize_temporaries() {
    temporaries::temp_character_set_1 = Character_set();
    temporaries::temp_character_set_2 = Character_set();
    temporaries::old_characters_on_paths_to_some_sink = Character_set();
    temporaries::old_characters_on_all_paths_to_lower_bound_levels = Character_set();
    temporaries::old_characters_on_paths_to_root = Character_set();
    temporaries::old_characters_on_all_paths_to_root = Character_set();
    temporaries::chaining_numbers = std::vector<int>(constants::alphabet_size);
    temporaries::node_character_count = std::vector<long>(constants::alphabet_size);
    temporaries::incoming_edge_character_count = std::vector<long>(constants::alphabet_size);
    temporaries::outgoing_edge_character_count = std::vector<long>(constants::alphabet_size);
}

void heuristic(instance &instance) {
    std::cout << "Heuristic started." << std::endl;
    std::flush(std::cout);
    heuristic_solve(instance);
}

void print_heuristic_stats(const instance &instance) {
    const std::chrono::duration<double> heuristic_elapsed_seconds = instance.heuristic_end - instance.start;
    std::cout << std::fixed << std::setprecision(2)
            << "Heuristic finished in " << heuristic_elapsed_seconds.count() << "s." << std::endl;
}

void reduction(instance &instance) {
    if (temporaries::lower_bound >= temporaries::upper_bound) {
        temporaries::upper_bound = temporaries::lower_bound;
        instance.active_matches = 0;
        return;
    }
    std::cout << "reduction is running." << std::endl;
    reduce_graph_pre_solver(instance);
    temporaries::upper_bound = std::max(temporaries::upper_bound, temporaries::lower_bound);
}

void solve(instance &instance) {
    if (temporaries::lower_bound >= temporaries::upper_bound) {
        std::cout << "Bounds converged, stopping solver." << std::endl;
        instance.end = instance.reduction_end;
        instance.active_matches = 0;
        instance.is_valid_solution = true;
        return;
    }

    std::cout << "Solver is running." << std::endl;

    if (instance.shared_object->is_mdd_reduction_complete) {
        solve_enumeration(instance);
        if (!instance.is_solving_forward) {
            std::ranges::reverse(instance.solution);
        }
    }

#ifdef ILP_FEATURE
     else if constexpr (SOLVER == GUROBI_MDD) {
        solve_gurobi_mdd_ilp(instance);
    } else if constexpr (SOLVER == GUROBI_MIS) {
        solve_gurobi_mis_ilp(instance);
    } else if constexpr (SOLVER == GUROBI_GRAPH) {
        solve_gurobi_graph_ilp(instance);
    } else if constexpr (SOLVER == GUROBI_MDD_EDGES) {
        solve_gurobi_mdd_edges_ilp(instance);
    }
#else
    else {
        std::cout << "ILP Solver deactivated." << std::endl;
    }
#endif

    if (int status; WIFEXITED(status)) {
        auto usage = rusage();
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            instance.main_process_memory_consumption = usage.ru_maxrss;
        } else {
            instance.main_process_memory_consumption = -1;
        }
    } else {
        instance.main_process_memory_consumption = -1;
    }
}

void check_solution(instance &instance) {
    auto characters = absl::flat_hash_set<int>();
    int position_1 = 0;
    int position_2 = 0;
    if (!instance.is_valid_solution) {
        std::cout << "Exact solver did not terminate.\t";
    }
    for (const auto character: instance.solution) {
        if (position_1 >= static_cast<int>(instance.string_1.size()) ||
            position_2 >= static_cast<int>(instance.string_2.size())) {
            std::cout << "String is out of available_characters.\t";
            instance.is_valid_solution = false;
            return;
        }
        if (characters.contains(character)) {
            std::cout << "Character " << character << " already in use.\t";
            instance.is_valid_solution = false;
            return;
        }
        characters.insert(character);
        position_1 = instance.next_occurrences_1[position_1 * constants::alphabet_size + character];
        position_2 = instance.next_occurrences_2[position_2 * constants::alphabet_size + character];
    }
    if (static_cast<int>(characters.size()) != temporaries::lower_bound) {
        std::cout << "Solution lower bound " << temporaries::lower_bound << " does not fit solution length of "
                << characters.size() << ".\t";
        instance.is_valid_solution = false;
        return;
    }
    if (position_1 > static_cast<int>(instance.string_1.size()) ||
        position_2 > static_cast<int>(instance.string_2.size())) {
        std::cout << "Strings are out of characters.\t";
        instance.is_valid_solution = false;
    }
}

void print_result_stats(const instance &instance) {
    const std::chrono::duration<double> reduction_seconds = instance.reduction_end - instance.start;
    const std::chrono::duration<double> solver_seconds = instance.end - instance.reduction_end;
    const std::chrono::duration<double> total_seconds = instance.end - instance.start;

    std::print("Total runtime: {:.2f}s = {:.2f}s + {:.2f}s\t",
               total_seconds.count(),
               reduction_seconds.count(),
               solver_seconds.count());
    std::print("bounds: {}/{}\t", temporaries::upper_bound, temporaries::lower_bound);
    std::print("solved: {}\t", instance.is_valid_solution ? "true" : "false");

    std::print("found solution: [");
    for (size_t i = 0; i < instance.solution.size(); ++i) {
        std::print("{}", instance.solution[i]);
        if (i + 1 < instance.solution.size()) {
            std::print(", ");
        }
    }
    std::print("]\n");
}
