/**
 * Author: Ethan Pinto
 * Student Number: s4642286
 * Program Name: jobrunner
 * File Name: parse.c
 *
 * FILE 2 OF 3
**/

#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <csse2310a3.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

                //* COMMAND LINE READING FUNCTIONS *//

/**
 * The check_usage function takes in the command line argument count
 * and the command line arguments, and checks the validity of the line.
 * It exits if: no command line arguments are specified, -v is the only
 * argument given, or if -v is not in the first position.
 * It returns a boolean which indicates if verbose mode is on or off.
 */
bool check_usage(int argc, char **argv) {
    bool verboseCheck = false;
    int verboseCount = 0;
    
    // Check if argument count is valid.
    if (argc < 2) {
        usage_err();
    }
    
    // Check for verbose argument/s.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            if (i != 1) {
                // -v is not in the first position.
                usage_err(); 
            } else {
                // Verbose mode is on.
                verboseCheck = true;
                verboseCount++;
            }
        }
    }

    // Check if multiple "-v" or only "-v" is present in the command line. 
    if (verboseCount > 1 || verboseCount == argc - 1) {
        usage_err();
    } 
    return verboseCheck;
}

/**
 * The check_command_line function tzkes in the argument count and the
 * command line arguments and looks at each input argument and checks
 * if they are valid inputs. It returns a pointer to a struct
 * containing information about the input arguments. It will exit 
 * with exit status 1 for a usage error, and 2 for an invalid file.
 */
CmdLineArgs *check_command_line(int argc, char **argv) {   
    // Check for usage errors in the command line arguments.
    bool verboseCheck = check_usage(argc, argv);

    // Create the Input Arguments array and set default values.
    CmdLineArgs *inputArgs = (CmdLineArgs *) malloc(sizeof(CmdLineArgs));
    inputArgs->jobFiles = (char **) malloc(sizeof(char *));
    inputArgs->jobNum = 0;
    inputArgs->verboseMode = verboseCheck;

    for (int j = 1; j < argc; j++) {
        if (strcmp(argv[j], "-v") != 0) {
            // Open each argument as if they were a file and check validity.
            FILE *jobFile = fopen(argv[j], "r");

            if (jobFile == NULL) {
                fprintf(stderr, "jobrunner: file \"%s\" can not be opened\n",
                        argv[j]);
                free(inputArgs->jobFiles);
                free(inputArgs);
                exit(2);

            } else if (jobFile) {
                fclose(jobFile);
                // Allocate enough memory to store job file name.
                inputArgs->jobFiles = realloc(inputArgs->jobFiles,
                        sizeof(char *) * (inputArgs->jobNum + 1));
            
                // Add the jobfile to the list of jobs.
                inputArgs->jobFiles[inputArgs->jobNum] = (char *) malloc(
                        strlen(argv[j]) + 1);
                strcpy(inputArgs->jobFiles[inputArgs->jobNum], argv[j]);
                inputArgs->jobNum++;
            }
        }
    }
    return inputArgs;
}

/**
 * The quick_free function will take in two strings (corresponding 
 * to a new line and a job file name), and will free the memory allocated.
 * It returns nothing.
 */ 
void quick_free(char *newLine, char *jobFile) {
    free(newLine);
    free(jobFile);
}

                //* ERROR HANDLING FUNCTIONS *//

/**
 * The usage_err function handles errors involving
 * invalid command line arguments. It returns nothing.
 */
void usage_err(void) {
    fprintf(stderr, "Usage: jobrunner [-v] jobfile [jobfile ...]\n");
    exit(1);
}

/**
 * The job_file_err takes in the line number, the line, and
 * the job file. It handles errors involving a syntactically
 * incorrect line within a jobfile. It will exit the program
 * with an exit status of 3.
 */ 
void job_file_err(int lineNum, char *newLine, char *jobFile) {
    fprintf(stderr, "jobrunner: invalid job specification on "
            "line %d of \"%s\"\n", lineNum, jobFile);
    quick_free(newLine, jobFile);
    exit(3);
}

/**
 * The open_err function checks for errors involving opening the
 * specified files for stdin and stdout for each job. It takes in
 * an integer corresponding to the filetype (stdin or stdout) and 
 * the job that must be checked, and checks if it's stdin and stdout
 * can be opened. If so, it stores the resulting file descriptor.
 * It returns nothing.
 */ 
void open_err(int fileType, Job *jobName) {  
    switch (fileType) {
        case STDIN: {
            // Checking if stdin file can be opened.
            int fdin = open(jobName->takeFrom, O_RDONLY, S_IRWXU);
            if (fdin < 0) {
                fprintf(stderr, "Unable to open \"%s\" for reading\n",
                        jobName->takeFrom);
                // Disable job so that it is not run.
                jobName->enabled = false;
            } else {
                jobName->inOutClose[0] = fdin;
            }
            break;
        }
        
        case STDOUT: {
            // Check if stdout file can be opened.
            int fdout = open(jobName->sendTo, O_CREAT | O_WRONLY | O_TRUNC,
                    S_IRWXU);
            if (fdout < 0) {
                fprintf(stderr, "Unable to open \"%s\" for writing\n",
                        jobName->sendTo);
                // Disable job so that it is not run.
                jobName->enabled = false;
            } else {
                jobName->inOutClose[1] = fdout;
            }
            break;
        }
    }
}
                
                    //* JOB FILE PARSING FUNCTIONS *//

/**
 * The count_args function takes in an array of strings, and 
 * will count and return the number of strings in the array.
 */ 
int count_args(char **args) {
    int count = 0;
    while (args[count]) {
        count++;
    }
    return count;
}

/**
 * The check_timeout function will check if all the characters in the
 * input argument (time) are positive integers. It returns 0 if the input
 * is a float, negative number or contains spaces, and 1 if input is valid.
 */ 
int check_timeout(char *time) {
    // Check if all characters in specified timeout argument are integers.
    for (int i = 0; i < strlen(time); i++) {
        if (isdigit(time[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * The add_job function takes in the jobList, jobCount and jobArgs, and
 * will add a new job to the jobList. The job parameters will be set to
 * the values provided in jobArgs, or will be set to default values.
 * It returns nothing.
 */ 
void add_job(Job **jobList, int jobCount, char **jobArgs) {
    jobList[jobCount] = (Job *) malloc(sizeof(Job));
    // Set parameters to default values..
    jobList[jobCount]->runningTime = 0;
    jobList[jobCount]->enabled = true;
    jobList[jobCount]->opArgs = NULL;
    jobList[jobCount]->terminated = false;
    jobList[jobCount]->jobPid = -1;

    // Set default input/output streams to be same as Jobrunner
    jobList[jobCount]->inOutClose[0] = STDIN;
    jobList[jobCount]->inOutClose[1] = STDOUT;
    jobList[jobCount]->inOutClose[2] = -1;

    // Allocate memory for mandatory arguments
    jobList[jobCount]->program = (char *) malloc(strlen(jobArgs[0]) + 1); 
    jobList[jobCount]->takeFrom = (char *) malloc(strlen(jobArgs[1]) + 1);
    jobList[jobCount]->sendTo = (char *) malloc(strlen(jobArgs[2]) + 1);
                
    // Copy value of arguments into allocated memory.
    strcpy(jobList[jobCount]->program, jobArgs[0]);
    strcpy(jobList[jobCount]->takeFrom, jobArgs[1]);
    strcpy(jobList[jobCount]->sendTo, jobArgs[2]);
}

/**
 * The free_jobs function takes in the jobList and the jobCount,
 * and frees the memory allocated to the Jobs in jobList and 
 * then frees the jobList array. It returns nothing.
 */ 
void free_jobs(Job **jobList, int jobCount) {
    for (int i = 0; i < jobCount; i++) {
        // Free mandatory arguments
        free(jobList[i]->program);
        free(jobList[i]->takeFrom);
        free(jobList[i]->sendTo);

        // Free optional arguments if present
        if (jobList[i]->opArgs) {
            int opArgCount = count_args(jobList[i]->opArgs);
            for (int j = 0; j < opArgCount; j++) {
                free(jobList[i]->opArgs[j]);
            }
            free(jobList[i]->opArgs);
        }
        free(jobList[i]);
    }
    free(jobList);
}

/**
 * The add_op_args function takes in the number of optional arguments
 * corresponding to the job given by jobName and the jobArgs. It checks
 * if there were optional arguments provided for the job, and subsequently
 * adds them to the specified job's (jobName) opArg array.
 * It returns nothing.
 */
void add_op_args(int opArgNum, Job *jobName, char **jobArgs) {
    if (opArgNum) {
    // Allocate memory for the optional arguments struct.
        jobName->opArgs = (char **) malloc(sizeof(char *) * opArgNum);
        for (int j = 0; j < opArgNum; j++) {
            jobName->opArgs[j] = (char *) malloc(strlen(jobArgs[j + 4]) + 1);
            strcpy(jobName->opArgs[j], jobArgs[j + 4]);
        }
    }
}

/**
 *  The is_empty function checks if a line in the jobfile is empty.
 *  It takes in the line and returns 0 if it is not empty and 1 if
 *  it is empty.
 */
int is_empty(char *line) {
    if (strlen(line)) {
        for (int i = 0; i < strlen(line); i++) {
            if (!isspace(line[i])) {
                // Line is not empty
                return 0;
            }
        }
    }
    // Line is empty
    return 1;
}

/**
 * The read_job_files function takes in a pointer to the struct
 * containing information about the input arguments (CmdLineArgs)
 * and a pointer to the job count. It reads and extracts any necessary
 * information from each specified jobfile. It returns an array of
 * pointers to jobs (called the jobList).
 */
Job **read_job_files(CmdLineArgs *inputArgs, int *jobCount) {
    int argCount;
    char *newLine;
    Job **jobList = (Job **) malloc(sizeof(Job *));

    for (int i = 0; i < inputArgs->jobNum; i++) {
        FILE *newFile = fopen(inputArgs->jobFiles[i], "r");
        int lineNum = 1;

        // Read and parse each line in the job file.
        while ((newLine = read_line(newFile))) { 
            if (newLine[0] == '#' || is_empty(newLine)) {
                lineNum++;
                free(newLine);
                continue;
            }
            // Get Job parameters and number of arguments.
            char **jobArgs = split_by_commas(newLine);
            argCount = count_args(jobArgs);
            
            // Handle Mandatory Command Line Arguments.
            if (argCount < 3 || !strlen(jobArgs[0]) ||
                    !strlen(jobArgs[1]) || !strlen(jobArgs[2])) {
                free(jobArgs);
                job_file_err(lineNum, newLine, inputArgs->jobFiles[i]);
            } else {
                jobList = realloc(jobList, sizeof(Job *) * (*jobCount + 1));
                add_job(jobList, *jobCount, jobArgs);
            }
           
            // Check validity of optional arguments and add them to Job.
            if (argCount > 3) {
                if (jobArgs[3] && check_timeout(jobArgs[3])) {
                    jobList[*jobCount]->runningTime = atoi(jobArgs[3]);
                } else {
                    free(jobArgs);
                    job_file_err(lineNum, newLine, inputArgs->jobFiles[i]);
                }
                add_op_args(argCount - 4, jobList[*jobCount], jobArgs);
            }    
            lineNum++;
            (*jobCount)++;
            free(jobArgs);
            free(newLine);
        } 
        fclose(newFile);
    }
    free_arr(inputArgs->jobNum, inputArgs->jobFiles);
    free(inputArgs);
    return jobList;
}

                //* JOB CHECKING FUNCTIONS *//

/**
 * The verbose_print function takes in the job List and job count.
 * It checks if the specified job has optional arguments, and prints
 * an output line of information about each job to the stderr buffer.
 * It returns nothing.
 */
void verbose_print(Job **jobList, int jobCount) {
    for (int i = 0; i < jobCount; i++) {
        // Check if the job is runnable and print information if it is.
        if (jobList[i]->enabled) {
            fprintf(stderr, "%d:%s:%s:%s:%d", i + 1, jobList[i]->program,
                    jobList[i]->takeFrom, jobList[i]->sendTo,
                    jobList[i]->runningTime);

            // Check for optional arguments, and print if present. 
            if (jobList[i]->opArgs) {
                int opArgNum = count_args(jobList[i]->opArgs);
                for (int j = 0; j < opArgNum; j++) {
                    // Add the optional argument to the end of the string.
                    fprintf(stderr, ":%s", jobList[i]->opArgs[j]);
                }
            }
            fprintf(stderr, "\n");
        } 
    }
}

/**
 * The free_arr function takes in an array and the number of
 * elements in the array and frees the memory allocated to 
 * each element and then frees the overall array.
 * It returns nothing.
 */
void free_arr(int num, char **elements) {
    for (int i = 0; i < num; i++) {
        free(elements[i]);
    }
    free(elements);
}

/**
 * The get_pipes function takes in two arrays: ins (containing all 
 * stdin files/pipes), and outs (containing all stdout files/pipes),
 * the job count, and a pointer to the pipe count. It creates and
 * returns an array called allPipes which contains the names of all
 * the pipes used in the jobfile.
 */
char **get_pipes(char **ins, char **outs, int jobCount, int *pipeCount) {
    char **allPipes = (char **) malloc(sizeof(char *));
    char **pipeList = (char **) malloc(sizeof(char *));
    int totPipes = 0;

    // Add pipe names from the stdin and stdout of each job in order.
    for (int i = 0; i < jobCount; i++) {
        // Add stdin pipes to array. 
        if (ins[i]) {
            pipeList = realloc(pipeList, sizeof(char *) * (totPipes + 1));
            pipeList[totPipes] = (char *) malloc(strlen(ins[i]) + 1);
            strcpy(pipeList[totPipes], ins[i]);
            totPipes++;
        }
        // Add stdout pipes to array. 
        if (outs[i]) {
            pipeList = realloc(pipeList, sizeof(char *) * (totPipes + 1));
            pipeList[totPipes] = (char *) malloc(strlen(outs[i]) + 1);
            strcpy(pipeList[totPipes], outs[i]);
            totPipes++;
        }
    }

    // Check to ensure each pipe name only appears once.
    for (int k = 0; k < totPipes; k++) {
        // Check if pipe is not NULL and add pipe name to allPipes.
        if (pipeList[k]) {
            allPipes = realloc(allPipes, sizeof(char *) * (*pipeCount 
                    + 1));
            allPipes[*pipeCount] = (char *) malloc(strlen(pipeList[k]) + 1);
            strcpy(allPipes[*pipeCount], pipeList[k]);
            
            // Set other pipes with same name to NULL
            for (int l = 0; l < totPipes; l++) {     
                // Check if the pipe is not NULL already.
                if (pipeList[l]) {
                    if (strcmp(pipeList[l], allPipes[*pipeCount]) == 0) {
                        free(pipeList[l]);
                        pipeList[l] = NULL;
                    }
                }
            }
            (*pipeCount)++;
        }
    }
    free(pipeList);
    return allPipes;
}

/**
 * The make_inout_arrs function takes in two arrays (ins and outs),
 * the jobList and jobCount and populates the arrays with the input
 * and output pipe of each job (if applicable) in the joblist. If a
 * job doesn't use pipes, a value of NULL will be added to the array.
 * It returns nothing.
 */ 
void make_inout_arrs(char **ins, char **outs, Job **jobList, int jobCount) { 
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i]->takeFrom[0] == '@') {
            ins[i] = (char *) malloc(strlen(jobList[i]->takeFrom) + 1);
            strcpy(ins[i], jobList[i]->takeFrom);
        } else {
            ins[i] = NULL;
        } 

        if (jobList[i]->sendTo[0] == '@') {
            outs[i] = (char *) malloc(strlen(jobList[i]->sendTo) + 1);
            strcpy(outs[i], jobList[i]->sendTo);
        } else {
            outs[i] = NULL;
        }
    } 
}

/**
 * The disable_jobs function takes in the name of a pipe, the job list,
 * and the job count, and checks all jobs in the joblist to see
 * if they are using an invalid pipe given by pipeName. If a job is
 * found, it is disabled. The function returns nothing.
 */
void disable_jobs(char *pipeName, Job **jobList, int jobCount) {
    for (int i = 0; i < jobCount; i++) {
        if (strcmp(pipeName, jobList[i]->takeFrom) == 0) {
            jobList[i]->enabled = false;
        }
        if (strcmp(pipeName, jobList[i]->sendTo) == 0) {
            jobList[i]->enabled = false;
        }
    }
}

/**
 * The check_cascade function handles cascading job invalidity. It takes in 
 * the ins and outs array, the job list, job count, pipe count and pipe array.
 * It searches ins and outs for linked jobs and checks if they are both
 * enabled. If not, both jobs will be disabled. It returns nothing.
 */
void check_cascade(char **ins, char **outs, Job **jobList, int jobCount,
        int pipeCount, char **allPipes) {
    
    // Iterate through each pipe to find linked jobs.
    for (int pipeNum = 0; pipeNum < pipeCount; pipeNum++) {
        int firstMatch, secondMatch;
        if (!allPipes[pipeNum]) {
            continue;
        }
        for (int jobNum = 0; jobNum < jobCount; jobNum++) {
            // Check stdin to see if pipe is used.
            if (ins[jobNum]) {
                if (strcmp(ins[jobNum], allPipes[pipeNum]) == 0) {
                    firstMatch = jobNum;
                }
            }
            // Check stdout to see if pipe is used.
            if (outs[jobNum]) {
                if (strcmp(outs[jobNum], allPipes[pipeNum]) == 0) {
                    secondMatch = jobNum;
                }
            }
        }   
        // Check if at least one of the jobs are disabled.
        if (!jobList[firstMatch]->enabled || !jobList[secondMatch]->enabled) {
            jobList[firstMatch]->enabled = false;
            jobList[secondMatch]->enabled = false;
        }
    }
}

/**
 * The check_pipes function takes in the jobList and jobCount and
 * checks the pipe usage in the jobs specified. It returns nothing.
 */ 
void check_pipes(Job **jobList, int jobCount) {    
    // Make arrays of stdin and stdout arguments.
    char **ins = (char **) malloc(sizeof(char *) * jobCount);
    char **outs = (char **) malloc(sizeof(char *) * jobCount);
    make_inout_arrs(ins, outs, jobList, jobCount);
        
    // Make an array of all Pipes mentioned in jobfile.
    int pipeCount = 0;
    char **allPipes = get_pipes(ins, outs, jobCount, &pipeCount);

    // Iterate through each pipe.
    for (int pipeNum = 0; pipeNum < pipeCount; pipeNum++) {
        int inMatches = 0;
        int outMatches = 0;

        // Check if pipe is used more than once in stdin and stdout.
        for (int jobNum = 0; jobNum < jobCount; jobNum++) {
            if (ins[jobNum]) {
                if (strcmp(ins[jobNum], allPipes[pipeNum]) == 0) {
                    inMatches++;
                }
            }
            if (outs[jobNum]) {
                if (strcmp(outs[jobNum], allPipes[pipeNum]) == 0) {
                    outMatches++;
                }
            }
        }   
        if (inMatches != 1 || outMatches != 1) {
            fprintf(stderr, "Invalid pipe usage \"%s\"\n", allPipes[pipeNum]
                    + 1);
            // Disable all jobs which use this pipe.
            disable_jobs(allPipes[pipeNum], jobList, jobCount);
            allPipes[pipeNum] = NULL;
        }
    }
    // Check for cascading job invalidity caused by invalid pipes and files.
    check_cascade(ins, outs, jobList, jobCount, pipeCount, allPipes);

    free_arr(jobCount, ins);
    free_arr(jobCount, outs);
    free_arr(pipeCount, allPipes);
}

/**
 * The check_jobs function takes in the job list, job count and a boolean
 * indicating if verbose mode is on. It iterates through each job in the
 * joblist and checks the validity of the stdin and stdout files provided.
 * It also oversees pipe error handling and checks the number of runnable
 * jobs. It exits with an exit status of 4 if there are no runnable jobs.
 * It returns nothing.
 */ 
void check_jobs(Job **jobList, int jobCount, bool verboseMode) {
    // For each job, check if normal stdin and stdout files can be opened.
    for (int i = 0; i < jobCount; i++) {
        if (strcmp(jobList[i]->takeFrom, "-") != 0 &&
                jobList[i]->takeFrom[0] != '@') {
            open_err(STDIN, jobList[i]);
        } 
        if (strcmp(jobList[i]->sendTo, "-") != 0 &&
                jobList[i]->sendTo[0] != '@') {
            open_err(STDOUT, jobList[i]);
        }
    }  
    // Check pipe usage.
    check_pipes(jobList, jobCount);

    // Check how many jobs can be run.
    int runnableJobs = 0;
    for (int k = 0; k < jobCount; k++) {
        if (jobList[k]->enabled) {
            runnableJobs++;
        }
    }

    if (!runnableJobs) {
        fprintf(stderr, "jobrunner: no runnable jobs\n");
        free_jobs(jobList, jobCount);
        exit(4);
    } else {
        if (verboseMode) {
            // Verbose mode is activated, so print job table.
            verbose_print(jobList, jobCount);
        }
    }
}
