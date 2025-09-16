#pragma once

#include <iosfwd>
#include <vector>

#include "character.hpp"
#include "instance.hpp"

enum PROCESSING_STATUS_CODE {
    SUCCESS,
    COMMAND_LINE_ERROR,
    INPUT_FILE_ERROR,
};

auto parse_next_integer(std::ifstream &input_file) -> int;

void parse_string(std::vector<Character> &character_sequence, std::ifstream &input_file, int string_length);

PROCESSING_STATUS_CODE process_input(instance &instance);

PROCESSING_STATUS_CODE process_command_line_arguments(int argc, char **argv, instance &instance);

std::string &get_default_output_path(std::string path);
