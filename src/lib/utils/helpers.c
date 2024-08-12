#include "../../../include/utils.h"

// replace char in string
void replace_char(char *str, char src_char, char new_char) {
    while(*str) {
        if (*str == src_char) *str = new_char;
        str ++;
    }
}
