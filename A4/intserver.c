/**
 *  Author: Ethan Pinto
 *  Student Number: s4642286
 *  Course: CSSE2310
 *  Program Name: intserver
**/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <tinyexpr.h>
#include <csse2310a4.h>
#include <csse2310a3.h>
#include "intserver.h"
#include "intclient.h"
#include "common.h"

/**
 * The usage_err function takes in no arguments. It handles usage 
 * errors in the command line arguments. It prints an error message
 * and exits with an exit status of 1.
 */
void usage_err(void) {
    fprintf(stderr, "Usage: intserver portnum [maxthreads]\n");
    exit(1);
}   

/**
 * The open_err functions takes in no arguments. It handles errors that
 * arise from attempting to open a specific port. It prints an error 
 * message to stderr, and exits the program with a status of 3.
 */ 
void open_err(void) {
    fprintf(stderr, "intserver: unable to open socket for listening\n");
    exit(3);   
}

/**
 * Checks if an input string (port) contains any letters. 
 * Returns 1 if port number contains letters.
 * Returns 0 if port number doesn't contain any letters.
 */
int alpha_check(char *port) {
    for (int i = 0; i < strlen(port); i++) {
        // Check if each character is a letter.
        if (isalpha(port[i]) != 0) {
            return 1;
        }
    } 
    // Port number only contains digits.
    return 0;
}

/**
 * The check_usage function will take in the argument count and 
 * command line arguments. It will check the arguments for usage
 * errors and will exit with a status of 1 if an error is found.
 * It returns a pointer to a Server Info struct if no errors are 
 * found.
 */ 
ServerInfo *check_usage(int argc, char **argv) {
    // Check if valid number of arguments were input.
    if (argc < MIN_SERVER_ARGS || argc > MAX_SERVER_ARGS) {
        usage_err();
    }
    
    ServerInfo *server = (ServerInfo *) malloc(sizeof(ServerInfo));

    // Check the validity of the port number given.
    int portNum = atoi(argv[1]);
    if (portNum < 0 || portNum > MAX_PORT_NUM || alpha_check(argv[1]) ||
            contains_whitespace(argv[1])) {
        free(server);
        usage_err();
    } else {
        server->port = (char *) malloc(strlen(argv[1] + 1));
        strcpy(server->port, argv[1]);
    }

    // Check for max threads argument.
    if (argc == MAX_SERVER_ARGS) {
        if (atoi(argv[2])) {
            if (atoi(argv[2]) < 0) {
                free(server);
                usage_err();
            } else {
                server->maxThreads = atoi(argv[2]);
            }
        } else {
            // Cannot convert to an integer.
            free(server);
            usage_err();
        }        
    }
    return server;
}

/**
 * The open_port function takes in a string corresponding to a port
 * number and will attempt to open the port. If successful, a socket
 * will be bound to it and the server will allowed to listen for
 * connections. The file descriptor that is used to listen for
 * connections will be returned by the function if no errors are
 * encountered. If an error is encountered, the program will exit with -1.
 */ 
int open_port(char *port) {
    struct addrinfo *ai = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &ai)) {
        freeaddrinfo(ai);
        exit(-1);
    }

    // Create the server socket.
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);

    // Enable the socket address to be reused.
    int optVal = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int))) {
        exit(-1);
    }

    // Bind the local address to the socket.
    if (bind(serverfd, (struct sockaddr*)ai->ai_addr,
            sizeof(struct sockaddr))) {
        open_err();
    }

    // Find the port number and output it to stderr.
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(serverfd, (struct sockaddr*)&ad, &len)) {
        exit(-1);
    }
    fprintf(stderr, "%u\n", ntohs(ad.sin_port));

    // Enable the server to listen for connection requests from client.
    if (listen(serverfd, CONNECTION_LIMIT) < 0) {
        exit(-1);
    }
    
    return serverfd;
}

                // Function Validation Handling Functions //

/**
 * The send_response takes in a boolean indicating if the response
 * required is for an OK request or for a Bad Request, and the client
 * file descriptor. Depending on the input parameter, the function 
 * will construct an HTTP response to the HTTP request from the client,
 * and will send the response to the client. It returns nothing.
 */
void send_response(bool responseType, int clientFd) {
    // Create response parameters
    int status;
    char *statusExplanation;
    HttpHeader **headers = NULL;
    char *body = NULL;
    
    if (responseType) { 
        // Create an OK response.
        status = OK_STATUS;
        statusExplanation = strdup("OK");
    } else {
        // Create a Bad Request response.
        status = BAD_STATUS;
        statusExplanation = strdup("Bad Request");
    }
    char *response = construct_HTTP_response(status, statusExplanation, 
            headers, body);
    send(clientFd, response, strlen(response), 0);
    free(response);
}

/**
 * check_function will take in a string corresponding to a function and
 * will try to compile it. It will return 1 if the function is valid,
 * and 0 if the function is invalid.
 */
int check_function(char *function) { 
    // Try to compile function and check if it is valid or not.
    double x;
    te_variable vars[] = {{"x", &x}};
    te_expr *expr = te_compile(function, vars, 1, NULL);

    if (expr) {
        // Function is valid
        te_free(expr);        
        return 1;
    } 

    // Function is invalid.
    return 0;
}

                    // Integration Request Functions //

/**
 * The check_validity function takes in a pointer to an integration job
 * struct and a pointer to a StrArgs struct. It will check the string form
 * of the provided arguments for syntax errors, then copy the parameters
 * into the intJob struct to be parsed for numerical validity. If the job
 * is valid (no errors were encountered), the function returns 0. If there
 * are errors with the parameters, the function returns 1.
 */
int check_validity(IntegrationJob *intJob, StrArgs *intArgs) {
    // Check if all parameters are non-empty.
    if (!strlen(intArgs->function) || !strlen(intArgs->lower) ||
            !strlen(intArgs->upper) || !strlen(intArgs->segments) ||
            !strlen(intArgs->threads)) {
        free(intArgs);
        return 1;
    }

    // Store the function in the integration job structure.
    intJob->function = strdup(intArgs->function);

    // Check validity of lower, upper, threads, and segments fields.
    int surplus[NUM_PARAMS];
    int countLower = sscanf(intArgs->lower, "%lf%n",
            &intJob->lower, &surplus[0]);
    int countUpper = sscanf(intArgs->upper, "%lf%n",
            &intJob->upper, &surplus[1]);
    int countSegs = sscanf(intArgs->segments, "%d%n",
            &intJob->segments, &surplus[2]);
    int countThread = sscanf(intArgs->threads, "%d%n",
            &intJob->threads, &surplus[3]);

    int total = countLower + countUpper + countSegs + countThread;

    if (total != NUM_PARAMS || intArgs->lower[surplus[0]]
            || intArgs->upper[surplus[1]]
            || intArgs->segments[surplus[2]]
            || intArgs->threads[surplus[3]]) { 
        free(intArgs);
        return 1; 
    }
    
    // Check validity of parameters
    if (contains_whitespace(intJob->function) || (intJob->threads < 0) ||
            (intJob->upper < intJob->lower) || (intJob->segments < 0) ||
            (intJob->segments % intJob->threads)) {
        return 1;
    }
    return 0;
}

/**
 * The get_str_args function takes in an array of strings which holds
 * the address information from the http response. It creates a pointer
 * to a StrArgs struct and stores the parameters from the http response
 * in it. It returns a pointe to an StrArgs struct.
 */ 
StrArgs *get_str_args(char **addressPars) {
    StrArgs *intArgs = (StrArgs *) malloc(sizeof(StrArgs));
    
    // Store parameters in intArgs.
    intArgs->lower = strdup(addressPars[2]);
    intArgs->upper = strdup(addressPars[3]);
    intArgs->segments = strdup(addressPars[4]);
    intArgs->threads = strdup(addressPars[5]);
    intArgs->function = strdup(addressPars[6]);

    return intArgs;
}

/**
 * The parse_int_request function takes in the request from the client,
 * the number of bytes that makes up the request, and a pointer to the 
 * integration job. It reads and parses the integration request. If the
 * request is valid, the parameters provided are also checked. If the
 * request was found to be invalid, the function will return 1. If the
 * request is valid, the function returns 0. 
 */ 
int parse_int_request(char *request, int numBytes, IntegrationJob *intJob) {
    // declare useful parameters to store parameters, and parse request.
    char *method = NULL;
    char *address = NULL;
    HttpHeader **headers = NULL;
    char *body = NULL;

    parse_HTTP_request(request, numBytes, &method, &address, &headers, &body);
   
    // Check for verbose mode.
    int index = 0;
    while (headers[index]) {
        if (strcmp(headers[index]->name, "X-Verbose") == 0 &&
                strcmp(headers[index]->value, "yes") == 0) {
            // Client has verbose mode activated.
            intJob->verboseMode = true;
            break;
        }
        index++;
    }
    
    // Check validity of parameters
    char **addressPars = split_by_char(address, '/',
            INT_REQUEST_ARG_NUM);       
    StrArgs *intArgs = get_str_args(addressPars);

    if (check_validity(intJob, intArgs)) {
        // Invalid request
        free(addressPars);
        free(intArgs);
        return 1;
    } 
    
    free(addressPars);
    free(intArgs);
    free(method);
    free(address);
    free(body);
    free_array_of_headers(headers);
    return 0;
}

                // Computation Thread Functions //

/**
 * The compute_area function is a thread handling function. It takes in
 * a void pointer and converts it to a pointer to a ThreadPars struct. It
 * then calculates the area of each segment allocated to the thread and
 * returns the total area calculated by the thread (casted to a void pointer).
 */
void *compute_area(void *pars) {
    // Cast argument back to struct pointer.
    ThreadPars *intPars = (ThreadPars *) pars;
    
    // Create an expression variable called 'x', and set it to lower bound.
    double x, y1, y2;
    te_variable vars[] = {{"x", &x}};
    x = intPars->lower;

    // Compile the provided function
    te_expr *expr = te_compile(intPars->function, vars, 1, NULL); 

    double *totalArea = (double *) malloc(sizeof(double));
    *totalArea = 0;

    for (int i = 1; i < intPars->segNum + 1; i++) { 
        y1 = te_eval(expr);
        x = intPars->lower + (i * intPars->width);
        y2 = te_eval(expr);
        
        // Add to the total area.
        *totalArea += intPars->width * ((y1 + y2) / (double) 2);
    }
    te_free(expr);
    return (void *)totalArea;
}

/**
 * The integrate function takes in a pointer to an Integration Job. It
 * calculates the parameters required to perform the integration, and then
 * spawns the computational threads needed to calculate the integral. It
 * returns a pointer to an array of doubles which holds the area calculated
 * by each thread.
 */
double *integrate(IntegrationJob *intJob) {
    // Determine width of each trapezoid
    double width = fabs(intJob->upper - intJob->lower) / (
            (double)intJob->segments);

    // Determine the number of segments to evaluate per trapezoid
    int segsPerThread = intJob->segments / intJob->threads;
    double lower = intJob->lower;

    // Create an array of doubles which holds the thread output values.
    double *retAreas = (double *) malloc(sizeof(double) * intJob->threads);

    // Create arrays to store thread information
    ThreadPars intPars[intJob->threads];
    pthread_t threadIds[intJob->threads];

    for (int i = 0; i < intJob->threads; i++) {
        intPars[i].function = strdup(intJob->function);
        intPars[i].width = width;
        intPars[i].segNum = segsPerThread;
        intPars[i].lower = lower;

        // Create a new computation thread
        pthread_create(&(threadIds[i]), 0, compute_area,
                (void *)(intPars + i));
        lower += (width * segsPerThread);
    }
    
    for (int i = 0; i < intJob->threads; i++) {
        void *retval;
        pthread_join(threadIds[i], &retval);
        retAreas[i] = *(double *)retval;
        free(retval);
    }

    for (int i = 0; i < intJob->threads; i++) {
        free(intPars[i].function);
    }

    return retAreas;
}

/**
 * The construct_int_response takes in a pointer to an Integration Job,
 * and a pointer to an array of doubles. It constructs an HTTP response
 * which contains all the information about the integration result. It
 * checks if verbose mode is active and if so, it adds extra thread info
 * to the body of the response. It returns the constructed http response.
 */
char *construct_int_response(IntegrationJob *intJob, double *retAreas) {
    // Create HTTP response parameters
    int status = OK_STATUS;
    char *statusExplanation = strdup("OK");
    HttpHeader **headers = NULL; 
    char body[BUFSIZE] = {0};

    if (intJob->verboseMode) {
        // Create body response
        double lower = intJob->lower;   
        double width = abs(intJob->upper - intJob->lower) / 
                ((double)intJob->segments);
        int segsPerThread = intJob->segments / intJob->threads;
        double threadWidth = width * segsPerThread;

        for (int i = 0; i < intJob->threads; i++) {
            char partial[BUFSIZE];
            sprintf(partial, PARTIAL_RESULT, i + 1, lower, lower + threadWidth,
                    retAreas[i]);
            lower += threadWidth;
            strcat(body, partial);
        }
    }
    // Calculate the total area under the given function.
    double totalArea = 0;
    for (int j = 0; j < intJob->threads; j++) {
        totalArea += retAreas[j];
    }
    char area[BUFSIZE];
    sprintf(area, "%lf\n", totalArea);
    strcat(body, area); 

    char *response = construct_HTTP_response(status, statusExplanation, 
            headers, body);
    return response;
}

/**
 * The check_request_type function takes in the client's request as
 * well as the number of bytes that is contained in the request. It
 * parses the request and returns 0 for a badly formed request, 1 for
 * an incomplete request, 2 for a valid (OK) request, 3 for an integration
 * request, and 4 for a well-formed but invalid request.
 */ 
int check_request_type(char *request, int numBytes) {
    int requestType;
    // Declare useful parameters to store request parameters
    char *method = NULL;
    char *address = NULL;
    HttpHeader **headers = NULL;
    char *body = NULL;

    // Check if client's request is complete.
    int charsRead = parse_HTTP_request(request, numBytes, &method, &address,
            &headers, &body);

    if (charsRead < 0) {
        // Client sent a badly formed request, so disconnect the client.
        requestType = 0;
    } else if (charsRead == 0) {
        // Client sent an incomplete request.
        requestType = 1;
    } else {
        // Check method is valid.
        if (strcmp(method, "GET") != 0) {
            return 4;
        }
        
        // Check valid requests
        char **requestPars = split_by_char(address, '/', FUN_REQUEST_ARG_NUM);
        if (strcmp(requestPars[1], "validate") == 0) {
            // Check function validity.
            if (check_function(requestPars[2])) {
                requestType = 2;
            } else {
                requestType = 4;
            } 
        } else if (strcmp(requestPars[1], "integrate") == 0) {
            requestType = 3;

        } else if ((strcmp(requestPars[1], "validate") != 0) && 
                (strcmp(requestPars[1], "integrate") != 0)) {
            // Request is well-formed but service is unavailable.
            requestType = 4;
        }
        free(requestPars);
        free(method);
        free(address);
        free(body);
        free_array_of_headers(headers);
    }
    return requestType;
}

/**
 * The handle_client thread function takes in a pointer to the the client
 * file descriptor. It waits for expression validation and integration
 * requests from the client and handles them appropriately. It returns
 * a void pointer.
 */ 
void *handle_client(void *connectedFd) {
    int clientFd = *(int *)connectedFd;
    free(connectedFd);
    char *request = (char *) malloc(sizeof(char) * BUFSIZE);
    int numBytes;

    while((numBytes = read(clientFd, request, BUFSIZE)) > 0) {
        int requestType = check_request_type(request, numBytes);
        request[numBytes] = '\0';

        // Check and handle and incomplete request.
        while(check_request_type(request, numBytes) == 1) {
            // Add another line to the current request.
            char extraReq[BUFSIZE];
            numBytes += read(clientFd, extraReq, BUFSIZE);
            strcat(request, extraReq);
        }

        // Should have a complete request by now.
        if (requestType == 0) { // Bad Request
            // Exit client thread.
            close(clientFd);
            pthread_exit(NULL);
        } else if (requestType == 2) { // Valid Request.
            send_response(OK, clientFd);
            continue;
        } else if (requestType == 4) { // Well-formed but invalid request
            send_response(BAD, clientFd); 
            continue;
       
        } else if (requestType == 3) { // Integration Request
            // Validate job parameters. 
            IntegrationJob *intJob = (IntegrationJob *) malloc(
                    sizeof(IntegrationJob));
            intJob->verboseMode = false;
                
            if (parse_int_request(request, numBytes, intJob)) {
                // Invalid parameters, send 'Bad Request' to client.
                send_response(BAD, clientFd); 
                continue;
            }
            // Construct and Send Integration result back to client.
            double *retAreas = integrate(intJob); 
            char *response = construct_int_response(intJob, retAreas);
            send(clientFd, response, strlen(response), 0);
            free(response);
        }
    }
    close(clientFd);
    pthread_exit(NULL);
}

/**
 * The process_connections function takes in the server file descriptor
 * and repeatedly accepts connections from clients. It spawns new threads
 * for each client that joins the server. It returns nothing.
 */ 
void process_connections(int serverFd) {
    int clientFd;

    // Repeatedly accept connections and fulfil client requests.
    while (1) {
        clientFd = accept(serverFd, NULL, NULL);

        // Make a pointer to the client's file descriptor
        int *newClient = (int *) malloc(sizeof(int));
        *newClient = clientFd;

	// Create a new thread to handle the client's requests.
        pthread_t threadId;
        pthread_create(&threadId, NULL, handle_client, newClient); 
        pthread_detach(threadId);	 
    }
    return;
}

/**
 * The entry point for the intserver program. It handles
 * the flow of the program.
 */ 
int main(int argc, char **argv) {
    // Check for usage errors.
    ServerInfo *server = check_usage(argc, argv);
   
    // Open the port number specified in the command line.
    int serverFd = open_port(server->port);

    // Accept and handle client connections and requests.
    process_connections(serverFd);

    return 0;
}
