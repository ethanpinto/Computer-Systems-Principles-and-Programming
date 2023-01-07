/**
 * Author: Ethan Pinto
 * Student Number: s4642286
 * Program Name: jobrunner
 * File Name: running.c
 * 
 * FILE 3 OF 3
**/

#include "parse.h"
#include "running.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>

// The receivedSig is a global variable which is used as
// a flag for SIGHUP signal handling.
extern bool receivedSig;

/**
 * The handle_sighup function is a function handler for
 * the sigaction struct that deals with the SIGHUP signal.
 * It takes in the signal number, and sets receivedSig to
 * be true. It returns nothing.
 */ 
void handle_sighup(int sig) {
    receivedSig = true;
}

                    //* PRE-RUNNING FUNCTIONS *//

/**
 * The create_pipes function takes in the job list and the job count.
 * It creates the pipes used by the jobs and stores the pipe file
 * descriptors in the file descriptor array (inOutClose) for each job.
 * It returns nothing.
 */
void create_pipes(Job **jobList, int jobCount) {
    // Make arrays of stdin and stdout arguments.
    char **ins = (char **) malloc(sizeof(char *) * jobCount);
    char **outs = (char **) malloc(sizeof(char *) * jobCount);
    make_inout_arrs(ins, outs, jobList, jobCount);  
    
    // Make an array of all Pipes mentioned in jobfile.
    int pipeCount = 0;
    char **allPipes = get_pipes(ins, outs, jobCount, &pipeCount);
    
    // Iterate through each pipe name and create a pipe.
    for (int pipeNum = 0; pipeNum < pipeCount; pipeNum++) {
        int fds[2];
        if (pipe(fds)) {
            // Pipe creation failed
            exit(-1);
        }
        
        // Find jobs which use this pipe and assign read/write end.
        for (int jobNum = 0; jobNum < jobCount; jobNum++) {
            if (ins[jobNum]) {
                if (strcmp(ins[jobNum], allPipes[pipeNum]) == 0) {  
                    // Store file descriptor for Job's stdin.
                    jobList[jobNum]->inOutClose[0] = fds[READ_END];
                    // Store end that should be closed.
                    jobList[jobNum]->inOutClose[2] = fds[WRITE_END];
                }
            }
            if (outs[jobNum]) {
                if (strcmp(outs[jobNum], allPipes[pipeNum]) == 0) {
                    // Store file descriptor for Job's stdout.
                    jobList[jobNum]->inOutClose[1] = fds[WRITE_END];
                    // Store end that should be closed.
                    jobList[jobNum]->inOutClose[2] = fds[READ_END];
                }
            }
        }   
    }
    free_arr(jobCount, ins);
    free_arr(jobCount, outs);
    free_arr(pipeCount, allPipes);
}

/**
 * The redirect function takes in the Job and the file descriptor for
 * stderr. It handles the redirection of stdin, stdout and stderr for
 * each job. It returns nothing.
 */
void redirect(Job *jobName, int nullFd) {
    // Redirect stdin and stdout.
    dup2(jobName->inOutClose[0], STDIN);
    dup2(jobName->inOutClose[1], STDOUT);

    // Surpress stderr
    dup2(nullFd, STDERR);
}

/**
 * The make_exec function takes in a job and returns an array
 * which comprises of the optional arguments of the job (if present),
 * along with the program name as the first entry, and NULL at the end.
 * This array will be passed to the execvp function.
 */ 
char **make_exec(Job *jobName) {
    char **execArgs = (char **) malloc(sizeof(char *));
    // Copy in the program name
    execArgs[0] = (char *) malloc(strlen(jobName->program) + 1);
    strcpy(execArgs[0], jobName->program);   

    if (jobName->opArgs) {
        int opArgCount = count_args(jobName->opArgs);
        execArgs = realloc(execArgs, sizeof(char *) * (opArgCount + 1) +
                sizeof(NULL));
   
        // Copy in the optional arguments if present.
        for (int i = 0; i < opArgCount; i++) {
            execArgs[i + 1] = (char *) malloc(strlen(jobName->opArgs[i]) + 1);
            strcpy(execArgs[i + 1], jobName->opArgs[i]);
        }
        // Set last argument to be NULL.
        execArgs[opArgCount + 1] = NULL;
    } else {
        execArgs = realloc(execArgs, sizeof(char *) + sizeof(NULL));
        // Set last argument to be NULL.
        execArgs[1] = NULL;
    }
    return execArgs;
}

                    //* RUNNING FUNCTIONS *//

/**
 * The moniter_jobs function takes in the job list, job count, and
 * pointers to the number of active jobs and seconds passed. It will
 * iterate through the joblist and using their assigned PIDs, will
 * identify their current status (terminated, exited, or running).
 * It also handles jobs that have a timeout.
 * It returns the number of active jobs.
 */
int moniter_jobs(Job **jobList, int jobCount, int *activeJobs, int *seconds) {
    int status;
    pid_t pid;
    for (int j = 0; j < jobCount; j++) {
        if (!jobList[j]->enabled) {
            continue;
        }
        // Check the current status of each job.
        if (((pid = waitpid(jobList[j]->jobPid, &status, WNOHANG))) == -1) {
            continue;
        } else if (pid == 0) {
            // Job is still running.
            if (jobList[j]->runningTime == -1) {
                // Job needs to be terminated
                kill(jobList[j]->jobPid, SIGKILL);

            } else if (jobList[j]->runningTime) {
                // Check if job has timed out.
                if (*seconds > jobList[j]->runningTime) {
                    // Send SIGABRT to job and set flag.
                    kill(jobList[j]->jobPid, SIGABRT);
                    jobList[j]->runningTime = -1;
                }
            }
        } else {
	    // Check what happened to Job.
            if (WIFEXITED(status)) {
                fprintf(stderr, "Job %d exited with status %d\n", (j + 1),
                        WEXITSTATUS(status));
                jobList[j]->terminated = true;
                (*activeJobs)--;
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "Job %d terminated with signal %d\n", (j + 1),
                        WTERMSIG(status));
                jobList[j]->terminated = true;
                (*activeJobs)--;
            }
        }
    }
    return *activeJobs;
}

/**
 * The close_fds function takes in the job list and job count
 * and closes all file descriptors that are used by each job.
 * It returns nothing.
 */
void close_fds(Job **jobList, int jobCount) {
    // Close all fds used for each job.
    for (int j = 0; j < jobCount; j++) {
        if (jobList[j]->inOutClose[2] != -1) {
            close(jobList[j]->inOutClose[2]);
        }
        if (jobList[j]->inOutClose[0] != STDIN) {
            close(jobList[j]->inOutClose[0]);
        }   
        if (jobList[j]->inOutClose[1] != STDOUT) {
            close(jobList[j]->inOutClose[1]);
        }
    }
}   

/**
 * The run_jobs function takes in the job list and job count.
 * It will iterate through the job list and create a new child process
 * for each job. It will run each job using the exec function, moniter
 * the status of each job, print an appropriate message regarding the
 * outcome of the job. The child process exits with a status of 255
 * if the exec call fails, and the program will exit with 0 after all
 * jobs have been run. It returns nothing.
 */ 
void run_jobs(Job **jobList, int jobCount) {
    // Find and store all the file descriptors.
    create_pipes(jobList, jobCount);
    int activeJobs = 0, seconds = 0;

    // Surpress stderr of all jobs
    int nullFd = open("/dev/null", O_WRONLY);

    // Create child processes to run each job.
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i]->enabled) {
            activeJobs++;
            
            if (!(jobList[i]->jobPid = fork())) {
                // Handle job's redirection and close all file descriptors.
                redirect(jobList[i], nullFd); 
                close_fds(jobList, jobCount);

                if (execvp(jobList[i]->program, make_exec(jobList[i])) < 0) {
                    // Exec call failed.
                    exit(255);
                }
            }
        }
    } 
    close_fds(jobList, jobCount);

    while (activeJobs) {
        // Check for a SIGHUP signal.
        if (receivedSig) {
            // Kill all enabled processes that have not terminated.
            for (int i = 0; i < jobCount; i++) {
                if (jobList[i]->enabled && !jobList[i]->terminated) {    
                    kill(jobList[i]->jobPid, SIGKILL);
                }
            }
        }
        // Moniter job status once every second.
        sleep(1);
        seconds++;
        activeJobs = moniter_jobs(jobList, jobCount, &activeJobs, &seconds);
    }

    free_jobs(jobList, jobCount);
    close(nullFd);
    exit(0);
}

