#ifndef _INTCLIENT_H
#define _INTCLIENT_H

#include <stdbool.h>

#define MAX_ARGS 4
#define MIN_ARGS 2
#define NUM_PARAMS 4
#define BAD_STATUS 400
#define OK_STATUS 200
#define BUFSIZE 10000
#define NEWLINE "\r\n"
#define VERBOSE_HEADER "X-Verbose: yes\r\n\r\n"
#define RESULT "The integral of %s from %lf to %lf is %s"

// Struct which stores information about the client.
typedef struct {
    bool verboseMode;   // True if verbose mode is enabled
    bool usingFile;     // True if a jobfile is present
    char *port;         // The port/service name that intserver is listening on
    char *jobFile;      // The name of the jobfile the client uses.
} ClientInfo;

#endif
