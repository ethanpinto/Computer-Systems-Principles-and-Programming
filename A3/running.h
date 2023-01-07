#ifndef _RUNNING_H
#define _RUNNING_H

// Macro Definitions
#define READ_END 0
#define WRITE_END 1

#include "parse.h"

// Function Declarations
void run_jobs(Job **jobList, int jobCount);
void handle_sighup(int sig); 

#endif
