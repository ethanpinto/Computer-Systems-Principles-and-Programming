#ifndef _COMMON_H
#define _COMMON_H

#include <stdbool.h>

// Struct which stores information about the integration job.
typedef struct {
    char *function;     // Integrand
    double lower;       // The lower bound of the integral
    double upper;       // The upper bound of the integral
    int segments;       // Number of trapezoidal segments used in integration
    int threads;        // Number of computation threads used in integration
    bool verboseMode;   // True if verbose mode is enabled
} IntegrationJob;

int contains_whitespace(char *function); 

#endif
