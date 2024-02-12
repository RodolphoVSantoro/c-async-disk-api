#ifndef HELPERS_H
#define HELPERS_H

// Header file for helper functions and constants
// Includes the countless standard libraries used in C socket programming

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// server port
#define SERVER_PORT 9999
// max connections waiting to be accepted
#define SERVER_BACKLOG 10
// 8KB
#define SOCKET_READ_SIZE 8 * 1024
// 16KB
#define RESPONSE_SIZE 16 * 1024
// 8KB
#define RESPONSE_BODY_SIZE 8 * 1024
// 4KB
#define RESPONSE_BODY_TRANSACTIONS_SIZE 4 * 1024

// Debug flags
// Comment out to enable logging
#define LOGGING 1
// Comment out to reset the database on every server startup
#define RESET_DB 1

// socket send default flag
#define SEND_DEFAULT 0
#define PROTOCOL_DEFAULT 0

// Custom error codes
#define ERROR -1
#define SUCCESS 0
#define FILE_NOT_FOUND -2

// Return error if pointer is NULL
#define errIfNull(pointer) \
    if (pointer == NULL) { \
        return ERROR;      \
    }

// Return error if the result is an error
#define raiseIfError(result) \
    if (result == ERROR) {   \
        return ERROR;        \
    }

// Return FILE_NOT_FOUND if the file pointer is NULL
#define raiseIfFileNotFound(filePointer) \
    if (filePointer == NULL) {           \
        return FILE_NOT_FOUND;           \
    }

// Response templates
const char* successResponseJsonTemplate = "HTTP/1.1 200 OK\nContent-Type: application/json\n\n%s";

// Send response to client
#define RESPOND(clientSocket, response) send(clientSocket, response, strlen(response), SEND_DEFAULT);

// static responses
// response must be a string literal
#define STATIC_RESPONSE(clientSocket, response) send(clientSocket, response, sizeof(response) - 1, SEND_DEFAULT);

const char badRequestResponse[] = "HTTP/1.1 400 Bad Request\nContent-Type: application/json\n\n{\"message\": \"Bad Request\"}";
#define BAD_REQUEST(clientSocket) STATIC_RESPONSE(clientSocket, badRequestResponse)

const char methodNotAllowedResponse[] = "HTTP/1.1 405 Method Not Allowed\nContent-Type: application/json\n\n{\"message\": \"Method not allowed\"}";
#define METHOD_NOT_ALLOWED(clientSocket) STATIC_RESPONSE(clientSocket, methodNotAllowedResponse)

const char notFoundResponse[] = "HTTP/1.1 404 Not Found\nContent-Type: application/json\n\n{\"message\": \"User Not Found\"}";
#define NOT_FOUND(clientSocket) STATIC_RESPONSE(clientSocket, notFoundResponse)

const char unprocessableEntityResponse[] = "HTTP/1.1 422 Unprocessable Entity\nContent-Type: application/json\n\n{\"message\": \"Unprocessable Entity\"}";
#define UNPROCESSABLE_ENTITY(clientSocket) STATIC_RESPONSE(clientSocket, unprocessableEntityResponse)

// HTTP methods
const char GET_METHOD[] = "GET";
const int GET_METHOD_LENGTH = sizeof(GET_METHOD) - 1;
const char POST_METHOD[] = "POST";
const int POST_METHOD_LENGTH = sizeof(POST_METHOD) - 1;

typedef struct sockaddr SA;
typedef struct sockaddr_in SA_IN;

// Check socket errors
// Crash the program if the expression evaluates to ERROR
int check(int expression, const char* message);

// Startup server socket on the given port, with the max number of connections waiting to be accepted set to backlog
// Crash the program if the socket creation or binding fails
int setupServer(short port, int backlog);

// Compare two strings up to maxLength
int partialEqual(const char* str1, const char* str2, int maxLength);

// Gets system time and stores it in timeStr
void getCurrentTimeStr(char* timeStr);

int check(int expression, const char* message) {
    if (expression == ERROR) {
        perror(message);
        exit(EXIT_FAILURE);
    }
    return expression;
}

int setupServer(short port, int backlog) {
    int serverSocket;
    check((serverSocket = socket(AF_INET, SOCK_STREAM, PROTOCOL_DEFAULT)), "Failed to create socket");

    SA_IN serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    //  To avoid "Address already in use" error when restarting the server because of the TIME_WAIT state
    // https://handsonnetworkprogramming.com/articles/bind-error-98-eaddrinuse-10048-wsaeaddrinuse-address-already-in-use/
    int yes = 1;
    check(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)), "Failed to set socket options");

    check(bind(serverSocket, (SA*)&serverAddress, sizeof(serverAddress)), "Failed to bind socket");
    check(listen(serverSocket, backlog), "Failed to listen on socket");

    return serverSocket;
}

int partialEqual(const char* str1, const char* str2, int maxLength) {
    for (int i = 0; i < maxLength; i++) {
        if (str1[i] == '\0' || str2[i] == '\0') {
            return false;
        }
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

void getCurrentTimeStr(char* timeStr) {
    time_t mytime = time(NULL);
    char* time_str = ctime(&mytime);
    time_str[strlen(time_str) - 1] = '\0';
    strcpy(timeStr, time_str);
}
#endif