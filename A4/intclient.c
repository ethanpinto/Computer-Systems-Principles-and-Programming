/**
 *  Author: Ethan Pinto
 *  Student Number: s4642286
 *  Course: CSSE2310
 *  Program Name: intclient
**/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <csse2310a4.h>
#include <csse2310a3.h>
#include <tinyexpr.h>
#include "intclient.h"
#include "common.h"

/**
 * The usage_err function handles errors involving 
 * invalid command line arguments. It exits the program
 * with an exit status of 1. It returns nothing.
 */
void usage_err(void) {
    fprintf(stderr, "Usage: intclient [-v] portnum [jobfile]\n");
    exit(1);
}

/**
 * The open_job_file function will take in a pointer to a ClientInfo
 * struct and a file with name fileName. It will check if that file can
 * be opened, and if not, it will exit with an exit status of 4. It
 * returns nothing.
 */ 
void open_job_file(ClientInfo *client, char *fileName) {
    // Check file can be opened.
    FILE *jobFile = fopen(fileName, "r");
    if (!jobFile) {    
        fprintf(stderr, "intclient: unable to open \"%s\" for reading\n",
                fileName);
        exit(4);
    } else {
        // File can be opened, so it is valid.
        client->usingFile = true;
        fclose(jobFile);
        client->jobFile = (char *) malloc(strlen(fileName) + 1);
        strcpy(client->jobFile, fileName);
    }
}

/**
 * The comms_err function takes in no arguments. It handles communication
 * errors between the client and the server. It prints a message to stderr
 * and exits with an exit code of 3. It returns nothing.
 */
void comms_err(void) {
    fprintf(stderr, "intclient: communications error\n");
    exit(3);
}

/**
 * The check_command_line function takes in the argument count
 * and command line arguments. It checks for usage errors and 
 * file opening errors and returns a pointer to a ClientInfo
 * struct.
 */ 
ClientInfo *check_command_line(int argc, char **argv) {
    // Check if argument count is valid.
    if (argc < MIN_ARGS || argc > MAX_ARGS) {
        usage_err();
    }

    // Make a ClientInfo struct for the client.
    ClientInfo *client = (ClientInfo *) malloc(sizeof(ClientInfo));
    client->verboseMode = false;
    client->usingFile = false;
    client->port = NULL;
    client->jobFile = NULL;
   
    // Check if verbose argument is not in the first position.
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 && i != 1) {
            usage_err();
        }
    }

    if (strcmp(argv[1], "-v") == 0) {
        if (argc == 2) {
            // No arguments follow "-v"
            usage_err();
        } else {
            client->port = (char *) malloc(strlen(argv[2]) + 1);
            strcpy(client->port, argv[2]);
            if (argc == 4) {
                open_job_file(client, argv[3]);
            }
            client->verboseMode = true;
        }
    } else {
        client->port = (char *) malloc(strlen(argv[1]) + 1);
        strcpy(client->port, argv[1]);    
        if (argc == 3) {
            open_job_file(client, argv[2]);
        }
    } 
    return client;
}

/**
 * The free_client function takes in a pointer to a ClientInfo
 * struct and frees the memory allocated to the elements in it.
 * It returns nothing.
 */ 
void free_client(ClientInfo *client) {
    if (client->port) {
        free(client->port);
    }
    if (client->jobFile) {
        free(client->jobFile);
    }
    free(client);
}

/**
 * The connect_to_server function takes in a pointer to a ClientInfo
 * struct. It will attempt to connect to the server on the port
 * number/service name provided in the command line. It will exit 
 * with an exit status of 2 if it cannot connect, and will exit with
 * 1 if there is an error connecting. It returns the file descriptor
 * of the newly created socket.
 */ 
int connect_to_server(ClientInfo *client) {
    struct addrinfo *ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("localhost", client->port, &hints, &ai)) {
        freeaddrinfo(ai);
        fprintf(stderr, "intclient: unable to connect to port %s\n",
                client->port);
        exit(2);
    }

    // Create a new socket and connect the client to it on the port number.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr *)ai->ai_addr, ai->ai_addrlen)) {
        freeaddrinfo(ai);
        fprintf(stderr, "intclient: unable to connect to port %s\n",
                client->port);
        exit(2);
    } 
    return sockfd;
}

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
 * The check_syntax function takes in a pointer to an Integration Job,
 * a string (newLine) and an integer which is the line number. It checks
 * each argument in newLine to see if there are any syntax errors. It
 * returns 1 if a syntax error is encountered, and 0 for no error.
 */
int check_syntax(IntegrationJob *intJob, char *newLine, int lineNum) {
    // Get Job parameters and number of arguments.
    char **intArgs = split_by_commas(newLine);
    int argCount = count_args(intArgs);
   
    // Check if there are 5 arguments and if all are non-empty.
    if (argCount != 5 || !strlen(intArgs[0]) || !strlen(intArgs[1]) ||
            !strlen(intArgs[2]) || !strlen(intArgs[3]) ||
            !strlen(intArgs[4])) {
        free(intArgs); 
        fprintf(stderr, "intclient: syntax error on line %d\n", lineNum);
        return 1;
    }

    // Store the function in the integration job structure.
    intJob->function = (char *) malloc(strlen(intArgs[0]) + 1);
    strcpy(intJob->function, intArgs[0]);

    // Check validity of lower, upper, threads, and segments fields.
    int surplus[NUM_PARAMS];

    // Parse each argument and check for surplus characters.
    int countLower = sscanf(intArgs[1], "%lf%n", &intJob->lower, &surplus[0]);
    int countUpper = sscanf(intArgs[2], "%lf%n", &intJob->upper, &surplus[1]);
    int countSegs = sscanf(intArgs[3], "%d%n", &intJob->segments, &surplus[2]);
    int countThrds = sscanf(intArgs[4], "%d%n", &intJob->threads, &surplus[3]);

    int total = countLower + countUpper + countSegs + countThrds;

    if (total != NUM_PARAMS || intArgs[1][surplus[0]] || intArgs[2][surplus[1]]
            || intArgs[3][surplus[2]] || intArgs[4][surplus[3]]) { 
        free(intArgs);
        fprintf(stderr, "intclient: syntax error on line %d\n", lineNum);
        return 1; 
    }

    // Check if the threads or segments are greater than INT_MAX
    char thread[BUFSIZE];
    char segment[BUFSIZE];
    sprintf(thread, "%d", intJob->threads);
    sprintf(segment, "%d", intJob->segments);
    
    if (strcmp(thread, intArgs[4]) != 0 || strcmp(segment, intArgs[3]) != 0) {
        free(intArgs);
        fprintf(stderr, "intclient: syntax error on line %d\n", lineNum);
        return 1; 
    }

    free(intArgs);
    return 0;
}

/**
 *  The is_empty function takes in a string corresponding to a line
 *  from a jobfile or stdin, and check if it is empty or not. It returns
 *  0 if it is not empty and 1 if it is empty.
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
 * The check_parameters function takes in a pointer to an integration
 * job struct and a line number and checks if the parameters specified
 * are valid. It returns 1 if there are invalid parameters and 0 if not.
 */
int check_parameters(IntegrationJob *intJob, int lineNum) {
    if (contains_whitespace(intJob->function)) {
        fprintf(stderr, "intclient: spaces not permitted in expression "
                "(line %d)\n", lineNum);
        return 1;

    } else if (intJob->upper <= intJob->lower) {
        fprintf(stderr, "intclient: upper bound must be greater than "
                "lower bound (line %d)\n", lineNum);
        return 1;

    } else if (intJob->segments <= 0) {
        fprintf(stderr, "intclient: segments must be a positive integer "
                "(line %d)\n", lineNum);
        return 1;

    } else if (intJob->threads <= 0) {
        fprintf(stderr, "intclient: threads must be a positive integer "
                "(line %d)\n", lineNum);
        return 1;

    } else if (intJob->segments % intJob->threads) {
        fprintf(stderr, "intclient: segments must be an integer multiple "
                "of threads (line %d)\n", lineNum);
        return 1;
    }
    return 0; 
}

                /** HTTP Request Constructing Function **/

/**
 * The construct_fun_request function takes in a char buffer and a
 * string corresponding to the function used in the integration. It
 * will copy the function into the function-checking HTTP request.
 * It returns the length of the request.
 */
int construct_fun_request(char *funReq, char *function) {
    // Copy the function into the request.
    sprintf(funReq, "GET /validate/%s HTTP/1.1\r\n\r\n", function);   
    return strlen(funReq);
}

/**
 * The construct_int_request function takes in a char buffer and
 * a pointer to an Integration Job. It copies all the integration
 * parameters into the char buffer which holds the HTTP request
 * for integration. It returns the length of the request.
 */
int construct_int_request(char *intReq, IntegrationJob *intJob) {
    // Copy the integration parameters into the HTTP request.
    sprintf(intReq, "GET /integrate/%lf/%lf/%d/%d/%s HTTP/1.1\r\n",
            intJob->lower, intJob->upper, intJob->segments, intJob->threads,
            intJob->function);    

    // Check if verbose mode header needs to be added in.
    if (intJob->verboseMode) {
        // Append the verbose header to the HTTP request.
        strcat(intReq, VERBOSE_HEADER); 
    } else {
        strcat(intReq, NEWLINE);
    }

    return strlen(intReq);
}

                /** HTTP Response Parsing Functions **/

/**
 * The parse_fun_response takes in the client's socket fd and will
 * parse the reponse sent to the client by the server regarding the
 * validity of the function. It returns 1 if the function is invalid
 * and returns 0 if the function is valid.
 */
int parse_function_response(int sockfd) {
    // Create buffer to store HTTP response information, and read response.
    char res[BUFSIZE];
    int charsRead;
    if ((charsRead = recv(sockfd, res, BUFSIZE, 0)) <= 0) {
        comms_err();
    }
    res[charsRead] = '\0';

    // Declare useful parameters to store information regarding response.
    int status;
    char *statusEx = NULL;
    HttpHeader **headers = NULL;
    char *body = NULL;
    char newbuf[BUFSIZE];
    int resCharsRead;
    
    while ((resCharsRead = parse_HTTP_response(res, charsRead, &status, 
            &statusEx, &headers, &body)) == 0) { 
        charsRead += read(sockfd, newbuf, BUFSIZE);
        strcat(res, newbuf);
    }

    // Check for badly formed response.
    if (resCharsRead == -1) {
        comms_err();
    }

    if (status == BAD_STATUS) {
        free(statusEx);
        free(body);
        free_array_of_headers(headers);
        return 1;
    }
    free(statusEx);
    free(body);
    free_array_of_headers(headers);
    return 0;
}

/**
 * The parse_int_response function takes in a pointer to the Integration
 * Job, and the client's socket fd. It parses the integration HTTP response
 * recieved from the server. It will print out important information to
 * stdout, and will return 1 if the integration fails, -1 for an unexpected
 * response, and 0 for no error.
 */
int parse_int_response(IntegrationJob *intJob, int sockfd) {
    // Create buffer to store HTTP response information, and read response.
    char res[BUFSIZE];
    int charsRead; 
    if ((charsRead = recv(sockfd, res, BUFSIZE, 0)) <= 0) {
        comms_err();
    }

    res[charsRead] = 0;
    // Declare useful parameters to store information regarding response.
    int status, resCharsRead;
    char *statusEx = NULL;
    HttpHeader **headers = NULL;
    char *body = NULL;
    char newbuf[BUFSIZE];
    
    while ((resCharsRead = parse_HTTP_response(res, charsRead, &status,
            &statusEx, &headers, &body)) == 0) { 
        int extraRead = read(sockfd, newbuf, BUFSIZE);
        charsRead += extraRead;
        newbuf[extraRead] = 0;
        strcat(res, newbuf);
    }   
    
    // Check for badly formed response.
    if (resCharsRead == -1) {
        comms_err();
    }   
    
    if (status == BAD_STATUS) {
        // The integration failed.
        return 1;
    } else if (status == OK_STATUS) {
        if (intJob->verboseMode) {
            char **partialRes = split_by_char(body, '\n', intJob->threads + 1);
            // Print partial results from each thread.
            for (int i = 0; i < intJob->threads; i++) {
                printf("%s\n", partialRes[i]);
            }
            printf(RESULT, intJob->function, intJob->lower, intJob->upper,
                    partialRes[intJob->threads]);
            free(partialRes);
        } else {
            printf(RESULT, intJob->function, intJob->lower, intJob->upper,
                    body);
        }
    } else if (status != OK_STATUS && status != BAD_STATUS) {
        return -1;
    }
    return 0;
}

                // HTTP Request and Response Handling //

/**
 * Check_function takes in a pointer to an integration job, the socket
 * file descriptor and the line number. It send a HTTP request to the
 * server to check if a function is valid and parses the response.
 * It returns 1 if the function is invalid, and 0 if the function is
 * valid.
 */ 
int check_function(IntegrationJob *intJob, int sockfd, int lineNum) {
    // Send HTTP Request to server to check validity of function.
    char funReq[BUFSIZE];
    int reqLen = construct_fun_request(funReq, intJob->function);
    
    if (send(sockfd, funReq, reqLen, 0) <= 0) {
        comms_err();
    }
    // Parse reponse from server.
    if (parse_function_response(sockfd)) {
        // Expression cannot be compiled
        fprintf(stderr, "intclient: bad expression \"%s\" (line %d)\n",
                intJob->function, lineNum);
        return 1;
    }    
    return 0;
}

/**
 * The handle_integration function takes in a pointer to an Integration
 * Job, the client's socket file descriptor, and the line number. It
 * contructs the HTTP integration requests, sends it to the server, and
 * parses the reponse from the server. It returns nothing.
 */ 
void handle_integration(IntegrationJob *intJob, int sockfd, int lineNum) {
    // Send HTTP Request to server for integration.
    char intReq[BUFSIZE];
    int reqLen = construct_int_request(intReq, intJob);
    
    if (send(sockfd, intReq, reqLen, 0) <= 0) {
        comms_err();
    }
    
    int outcome = parse_int_response(intJob, sockfd); 

    if (outcome == 1) { 
        fprintf(stderr, "intclient: integration failed\n");
    } else if (outcome == -1) {
        comms_err();
    }
}

                // Main Communication Functions //

/**
 * The communicate_with_server function takes in a pointer to the client
 * information struct, the client's socket file descriptor, and the file
 * stream that the client reads from. It reads each line of the file stream,
 * checks the validity of the line, and then handles the HTTP communication
 * with the server. It returns nothing.
 */ 
void communicate_with_server(ClientInfo *client, int sockfd, FILE *readFrom) {
    // Check if client uses a jobfile or stdin.
    char *newLine;
    int lineNum = 1;
    while ((newLine = read_line(readFrom))) { 
        // Check if line is a comment, and if so, ignore it.
        if (newLine[0] == '#' || is_empty(newLine)) {
            lineNum++;
            free(newLine);
            continue;
        }
        IntegrationJob *intJob = (IntegrationJob *) malloc(sizeof(
                IntegrationJob));
        intJob->verboseMode = client->verboseMode;
         
        // Check for syntax errors and validity of parameters.
        if (check_syntax(intJob, newLine, lineNum) || 
                check_parameters(intJob, lineNum) ||
                check_function(intJob, sockfd, lineNum)) {
            lineNum++;
            free(newLine);
            free(intJob);
            continue;
        }

        // Send request to server to evaluate integral.
        handle_integration(intJob, sockfd, lineNum);
        
        // Continue with the next integration job.
        lineNum++; 
        free(newLine);
        free(intJob);
    }
    // All integration jobs should have been completed by now.
    free_client(client);
}

/**
 * Entry point to the intclient program.
 * Handles the program's direction of flow.
 */ 
int main(int argc, char **argv) {
    // Check the command line for errors, and connect to the server.
    ClientInfo *client = check_command_line(argc, argv);
    int sockfd = connect_to_server(client);

    if (client->usingFile) {
        // Read from a jobfile
        FILE *jobFile = fopen(client->jobFile, "r");
        communicate_with_server(client, sockfd, jobFile);
        fclose(jobFile);
        close(sockfd);
        exit(0);
    } else {
        // Read from stdin
        communicate_with_server(client, sockfd, stdin);
        close(sockfd);
        exit(0);
    } 
    return 0;
}
