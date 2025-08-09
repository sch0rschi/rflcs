#include "formatting_utils.h"

char format_as_character(const Character character) {
    if(character == MAX_CHARACTER) {
        return 'r';
    }
    return 'a' + character;
}
