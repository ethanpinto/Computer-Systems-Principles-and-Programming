#ifndef _PARSE_H
#define _PARSE_H

#include <stdbool.h>
#include <unistd.h>

// Macro Definitions
#define STDIN 0
#define STDOUT 1
#define STDERR 2

// Define Structure to Organise Command Line Arguments
typedef struct {
    int jobNum;         // Number of Job Files
    bool verboseMode;   // True if Verbose mode is ON
    char **jobFiles;    // Array of job file names
} CmdLineArgs;

// Define Structure to Organise Job Information
typedef struct {
    char *program;      // Program Name
    char *takeFrom;     // Standard Input
    char *sendTo;       // Standard Output
    int runningTime;    // Time until Timeout
    bool enabled;       // True if Job can be run
    int inOutClose[3];  // Fds for stdin, stdout and to close if needed.
    pid_t jobPid;       // The PID assigned to job if it is enabled.
    bool terminated;    // True if the job has been terminated.
    char **opArgs;      // Optional Arguments
} Job;

// Function Declarations
CmdLineArgs *check_command_line(int argc, char **argv);
void usage_err(void);
Job **read_job_files(CmdLineArgs *inputArgs, int *jobCount); 
void free_jobs(Job **jobList, int jobCount); 
int count_args(char **args); 
void check_jobs(Job **jobList, int jobCount, bool verboseMode); 
void make_inout_arrs(char **ins, char **outs, Job **jobList, int jobCount);
char **get_pipes(char **ins, char **outs, int jobCount, int *pipeCount);
void free_arr(int num, char **elements);

#endif
