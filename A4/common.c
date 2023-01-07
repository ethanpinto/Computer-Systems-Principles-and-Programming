#include <string.h>
#include <ctype.h>
#include "common.h"

/**
 * The contains_whitespace function takes in a string representing
 * a function and returns 1 if the function contains whitespace
 * and 0 if the function doesn't contain whitespace.
 */
int contains_whitespace(char *function) {
    if (strlen(function)) {
        for (int i = 0; i < strlen(function); i++) {
            if (isspace(function[i])) {
                // Function contains whitespace
                return 1;
            }
        }
    }
    // Function does not contain whitespace.
    return 0;
}
