#include "config.hpp"
#include "temporaries.hpp"
#include "graph/header/graph_creation.hpp"
#include "heuristic.hpp"
#include "ilp_solver/ilp_solvers.hpp"
#include "instance.hpp"
#include "reduction_orchestration.hpp"
#include "result_writer.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <set>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <memory>
#include <sys/resource.h>
#include <boost/program_options.hpp>

auto parse_next_integer(std::ifstream &input_file) -> int;

void parse_string(std::vector<Character> &character_sequence, std::ifstream &input_file, int string_length);

void check_solution(instance &instance);

void heuristic(instance &instance);

void reduction(instance &instance);

void solve(instance &instance);

void process_input(instance &instance);

void solve_enumeration(instance &instance);

std::string &replace_instances_with_results_folder(std::string path);

auto main(const int argc, char **argv) -> int {
    try {
        auto command_line_description = boost::program_options::options_description("Allowed options");
        command_line_description.add_options()
                ("help,h", "Show this help message")
                ("input,i", boost::program_options::value<std::string>()->default_value("../RFLCS_instances/type1/512_8reps.24"), "Input file path")
                ("output,o", boost::program_options::value<std::string>(), "Output file path")
                ("reductiontimeout,r", boost::program_options::value<int>()->default_value(1800), "Reduction timeout [s]")
                ("solvertimeout,s", boost::program_options::value<int>()->default_value(1800), "Solver timeout [s]");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, command_line_description), vm);
        boost::program_options::notify(vm);

        if (vm.contains("help")) {
            std::cout << command_line_description << "\n";
            return 0;
        }

        instance instance;

        instance.input_path = vm["input"].as<std::string>();
        instance.output_path =
                vm.contains("output")
                    ? vm["output"].as<std::string>()
                    : replace_instances_with_results_folder(instance.input_path);
        constants::reduction_timeout = vm["reductiontimeout"].as<int>();
        constants::solver_timeout = vm["solvertimeout"].as<int>();

        process_input(instance);

        if (instance.input_validity_code != 0) {
            return instance.input_validity_code;
        }

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
        temporaries::int_vector_positions_2 = std::vector<int>();

        instance.start = std::chrono::system_clock::now();
        create_graph(instance);

        heuristic(instance);
        instance.heuristic_solution_length = temporaries::lower_bound;
        instance.heuristic_end = std::chrono::system_clock::now();
        const std::chrono::duration<double> heuristic_elapsed_seconds = instance.heuristic_end - instance.start;
        std::cout << std::fixed << std::setprecision(2)
                << "Heuristic finished in " << heuristic_elapsed_seconds.count() << "s." << std::endl;

        instance.mdd_node_source = std::make_unique<mdd_node_source>();

        reduction(instance);
        instance.reduction_end = std::chrono::system_clock::now();
        instance.reduction_upper_bound = temporaries::upper_bound;

        solve(instance);
        instance.end = std::chrono::system_clock::now();
        temporaries::upper_bound = std::max(temporaries::upper_bound, temporaries::lower_bound);

        check_solution(instance);
        write_result_file(instance);

        const std::chrono::duration<double> solve_elapsed_seconds = instance.end - instance.reduction_end;
        const std::chrono::duration<double> elapsed_seconds = instance.end - instance.start;
        std::cout << std::fixed << std::setprecision(2)
                << solve_elapsed_seconds.count() << "\t"
                << temporaries::lower_bound << "\t"
                << (instance.is_valid_solution ? "true" : "false") << "\t"
                << elapsed_seconds.count() << "\t"
                << "found solution: [";
        for (const auto character: instance.solution) {
            std::cout << character << ", ";
        }
        std::cout << "]" << std::endl;

        return 0;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

void process_input(instance &instance) {

    std::ifstream input_file(instance.input_path);

    std::cout << "solving file: " << instance.input_path << std::endl;
    std::flush(std::cout);

    if (input_file.is_open()) {
        if (int const number_of_strings = parse_next_integer(input_file); 2 != number_of_strings) {
            std::cout << "Only instances with two strings are allowed." << std::endl;
            instance.input_validity_code = 1;
        } else {
            constants::alphabet_size = parse_next_integer(input_file);
            temporaries::upper_bound = constants::alphabet_size;
            temporaries::lower_bound = 0;
            const auto string_1_length = parse_next_integer(input_file);
            parse_string(instance.string_1, input_file, string_1_length);
            const auto string_2_length = parse_next_integer(input_file);
            parse_string(instance.string_2, input_file, string_2_length);
        }
    } else {
        std::cout << "Cannot find input file: " << instance.input_path << "." << std::endl;
        instance.input_validity_code = 1;
    }

    input_file.close();
}

void heuristic(instance &instance) {
    std::cout << "Heuristic started." << std::endl;
    std::flush(std::cout);
    heuristic_solve(instance);
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
        instance.end = std::chrono::system_clock::now();
        instance.active_matches = 0;
        instance.is_valid_solution = true;
        return;
    }

    std::cout << "Solver is running." << std::endl;

    if constexpr (SOLVER == ENUMERATION) {
        solve_enumeration(instance);
        if (!instance.is_solving_forward) {
            std::ranges::reverse(instance.solution);
        }
    }

    if constexpr (SOLVER == GUROBI_MDD) {
        solve_gurobi_mdd_ilp(instance);
    }

    if constexpr (SOLVER == GUROBI_MIS) {
        solve_gurobi_mis_ilp(instance);
    }

    if constexpr (SOLVER == GUROBI_GRAPH) {
        solve_gurobi_graph_ilp(instance);
    }


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
    auto characters = std::set<int>();
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

void parse_string(std::vector<Character> &character_sequence, std::ifstream &input_file, const int string_length) {
    character_sequence.resize(string_length);
    for (int i = 0; i < string_length; i++) {
        character_sequence.at(i) = parse_next_integer(input_file);
    }
}

auto parse_next_integer(std::ifstream &input_file) -> Character {
    if (input_file.good()) {
        std::string file_entry; // NOLINT(*-const-correctness)
        input_file >> file_entry;
        return std::stoi(file_entry);
    }
    exit(1);
}

std::string &replace_instances_with_results_folder(std::string path) {
    const std::string from = "RFLCS_instances";
    const std::string to = "results";
    if (const size_t start_pos = path.find(from); start_pos != std::string::npos) {
        path.replace(start_pos, from.length(), to);
    }
    return path.append(".out");
}
