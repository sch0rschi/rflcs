#include "input_processing.hpp"

#include <fstream>

#include <boost/program_options.hpp>

PROCESSING_STATUS_CODE process_command_line_arguments(const int argc, char **argv, instance &instance) {
    auto command_line_description = boost::program_options::options_description("Allowed options");
    command_line_description.add_options()
            ("help,h", "Show this help message")
            ("input,i", boost::program_options::value<std::string>()->default_value("RFLCS_instances/type1/512_8reps.24"), "Input file path")
            ("output,o", boost::program_options::value<std::string>(), "Output file path")
            ("reductiontimeout,r", boost::program_options::value<int>()->default_value(1800), "Reduction timeout [s]")
            ("solvertimeout,s", boost::program_options::value<int>()->default_value(1800), "Solver timeout [s]");

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, command_line_description), vm);
    boost::program_options::notify(vm);

    if (vm.contains("help")) {
        std::cout << command_line_description << "\n";
        return COMMAND_LINE_ERROR;
    }

    instance.input_path = vm["input"].as<std::string>();
    instance.output_path =
            vm.contains("output")
                ? vm["output"].as<std::string>()
                : get_default_output_path(instance.input_path);
    constants::reduction_timeout = vm["reductiontimeout"].as<int>();
    constants::solver_timeout = vm["solvertimeout"].as<int>();
    return SUCCESS;
}

std::string &get_default_output_path(std::string path) {
    const std::string from = "RFLCS_instances";
    const std::string to = "results";
    if (const size_t start_pos = path.find(from); start_pos != std::string::npos) {
        path.replace(start_pos, from.length(), to);
    }
    return path.append(".out");
}

PROCESSING_STATUS_CODE process_input(instance &instance) {

    std::ifstream input_file(instance.input_path);

    std::cout << "solving file: " << instance.input_path << std::endl;
    std::flush(std::cout);

    if (input_file.is_open()) {
        if (int const number_of_strings = parse_next_integer(input_file); 2 != number_of_strings) {
            std::cout << "Only instances with two strings are allowed." << std::endl;
            instance.input_validity_code = 1;
            return INPUT_FILE_ERROR;
        }
        constants::alphabet_size = parse_next_integer(input_file);
        temporaries::upper_bound = constants::alphabet_size;
        temporaries::lower_bound = 0;
        const auto string_1_length = parse_next_integer(input_file);
        parse_string(instance.string_1, input_file, string_1_length);
        const auto string_2_length = parse_next_integer(input_file);
        parse_string(instance.string_2, input_file, string_2_length);
    } else {
        std::cout << "Cannot find input file: " << instance.input_path << "." << std::endl;
        instance.input_validity_code = 1;
        return INPUT_FILE_ERROR;
    }

    input_file.close();
    return SUCCESS;
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
    // TODO: handle this correctly
    exit(1);
}
