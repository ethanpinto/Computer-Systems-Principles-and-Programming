#ifndef _INTSERVER_H
#define _INTSERVER_H

#include <stdbool.h>

#define BUFSIZE 10000
#define MIN_SERVER_ARGS 2
#define MAX_SERVER_ARGS 3
#define MAX_PORT_NUM 65535
#define INT_REQUEST_ARG_NUM 7
#define FUN_REQUEST_ARG_NUM 3
#define CONNECTION_LIMIT 10
#define OK true
#define BAD false
#define DOUBLE_NEWLINE "\r\n\r\n"
#define PARTIAL_RESULT "thread %d:%lf->%lf:%lf\n"

// Contains information about the running server.
typedef struct {
    char *port;
    int maxThreads;
} ServerInfo;

// Contains information about the integration arguments.
typedef struct {
    char *function;
    char *lower;
    char *upper;
    char *segments;
    char *threads;
} StrArgs;

// Contains information about the thread parameters.
typedef struct {
    char *function;        // Function to be integrated (integrand)
    double width;          // Width of each trapezoid
    double lower;          // The lower bound of the integration segment. 
    int segNum;            // Number of integration segments (trapezoids)
} ThreadPars;

#endif
