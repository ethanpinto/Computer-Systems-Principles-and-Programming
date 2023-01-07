/**
 * Author: Ethan Pinto
 * Student Number: s4642286
 * Program Name: jobrunner
 * File Name: main.c
 *
 * FILE 1 OF 3
**/

#include "parse.h"
#include "running.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

// Global Variable Declaration for Signal Handling (SIGHUP)
bool receivedSig = false;

/**
 * Entry point to the jobrunner program.
 * Handles the program's direction of flow.
 */ 
int main(int argc, char **argv) {
    // Create a sigaction struct that handles a SIGHUP signal.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_sighup;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    // Examine the command line arguments.
    CmdLineArgs *inputArgs = check_command_line(argc, argv);
    bool verboseMode = inputArgs->verboseMode;
    
    // Load and parse job files and create array of jobs.
    int jobCount = 0;
    Job **jobList = read_job_files(inputArgs, &jobCount);

    // Check the list of jobs for runnability, and disable unrunnable jobs.
    check_jobs(jobList, jobCount, verboseMode);
    
    // Run all the jobs listed in the array of jobs (jobList).
    run_jobs(jobList, jobCount);
    
    return 0;
}
    
