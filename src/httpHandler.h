#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

// Header file for the http handler
// Handles the http requests and responses
// Handles deserialization and serialization of the requests and responses
// Calls the database functions to handle the requests

#include "dbFiles.h"

// Handles the request and sends the response to the clientSocket
int handleRequest(char* request, int requestSize, int clientSocket);

// Handles any GET request, assuming all GET requests are for the extract endpoint
int handleGetRequest(int clientSocket, char* request, int requestSize);
// Assuming the request is "GET /clientes/1/..." id is on the 14th position
// Returns ERROR if the request is invalid
// Returns the id if the request is valid
int getIdFromGETRequest(const char* request, int requestLength);
// Serializes GET extract response into json and writes it to response
void serializeGetResponse(User* user, char* response);

// Handles any POST request, assuming all POST requests are for creating transactions
int handlePostRequest(int clientSocket, char* request, int requestSize);
// Assuming the request is "POST /clientes/1/..." id is on the 15th position
// Returns ERROR if the request is invalid
// Returns the id if the request is valid
int getIdFromPOSTRequest(const char* request, int requestLength);
// Get transaction from body
// returns ERROR if the body it fails to parse the body
// returns SUCCESS if it parses the body successfully
// Sets the transaction variable with the parsed values
// Sets the transaction.realizada_em with the current time
int getTransactionFromBody(char* request, Transaction* transaction);
// Serializes POST transaction response into json and writes it to response
void serializePostResponse(User* user, char* response);

int handleRequest(char* request, int requestSize, int clientSocket) {
#ifdef LOGGING
    const char separator[] = "\n----------------------------------------------\n";

    printf("{ Received:");
    puts(separator);
    printf("[%s]", request);
    puts(separator);
    printf("(%d bytes read) }\n", requestSize);
#endif

    // "GET" alone has 3 bytes, so we need at least 4 bytes to consume a request
    if (requestSize < 4) {
        return BAD_REQUEST(clientSocket);
    }

    bool isGet = partialEqual(request, GET_METHOD, GET_METHOD_LENGTH);
    if (isGet) {
        return handleGetRequest(clientSocket, request, requestSize);
    }

    bool isPost = partialEqual(request, POST_METHOD, POST_METHOD_LENGTH);
    if (isPost) {
        return handlePostRequest(clientSocket, request, requestSize);
    }

    return METHOD_NOT_ALLOWED(clientSocket);
}

int handleGetRequest(int clientSocket, char* request, int requestSize) {
    // get id from request path
    int id = getIdFromGETRequest(request, requestSize);
    if (id == ERROR) {
        return NOT_FOUND(clientSocket);
    }

    // get user from db by id
    User user;
    int readResult = readUser(&user, id);
    if (readResult == ERROR) {
        return NOT_FOUND(clientSocket);
    }

    // serialize user to response
    char response[RESPONSE_SIZE];
    serializeGetResponse(&user, response);

    // send response
    return RESPOND(clientSocket, response);
}

int getIdFromGETRequest(const char* request, int requestLength) {
    if (requestLength < 15) {
        return ERROR;
    }
    if (request[13] != '/' || request[15] != '/') {
        return ERROR;
    }
    if (request[14] < '0' || request[14] > '9') {
        return ERROR;
    }
    return request[14] - '0';
}

void serializeGetResponse(User* user, char* response) {
    char body[RESPONSE_BODY_SIZE] = "";
    char transactionData[RESPONSE_BODY_TRANSACTIONS_SIZE];
    char dateTime[DATE_SIZE];

    // First part of the response
    getCurrentTimeStr(dateTime);
    const char* userDataTemplate = "{\"saldo\":%d,\"data_extrato\":\"%s\",\"limite\":%d,\"ultimas_transacoes\":[";
    sprintf(transactionData,
            userDataTemplate,
            user->total, dateTime, user->limit);
    strcat(body, transactionData);

    Transaction orderedTransactions[10];
    getOrderedTransactions(user, orderedTransactions);

    // Transactions array
    const char* transactionTemplate = "{\"valor\":%d,\"tipo\":\"%c\",\"descricao\":\"%s\",\"realizada_em\":\"%s\"}";
    for (int i = 0; i < user->nTransactions; i++) {
        Transaction transaction = orderedTransactions[i];

        sprintf(transactionData,
                transactionTemplate,
                transaction.valor, transaction.tipo, transaction.descricao, transaction.realizada_em);

        strcat(body, transactionData);

        if (i < user->nTransactions - 1) {
            strcat(body, ",");
        }
    }
    // Close the array and the outermost object
    strcat(body, "]}");

    // Write the http response using a success template, with a body
    sprintf(response, successResponseJsonTemplate, body);
#ifdef LOGGING
    printf("{ response: %s }\n", response);
#endif
}

int handlePostRequest(int clientSocket, char* request, int requestSize) {
    // get id from request path
    int id = getIdFromPOSTRequest(request, requestSize);
    if (id == ERROR) {
        return NOT_FOUND(clientSocket);
    }

    Transaction transaction;
    int parseResult = getTransactionFromBody(request, &transaction);
    if (parseResult == ERROR) {
        return BAD_REQUEST(clientSocket);
    }

    // update user on db by id
    User user;
    int transactionResult = updateUserWithTransaction(id, &transaction, &user);

    if (transactionResult == ERROR) {
        return UNPROCESSABLE_ENTITY(clientSocket);
    }

    // serialize user to response
    char response[RESPONSE_SIZE];
    serializePostResponse(&user, response);

    // send response
    return RESPOND(clientSocket, response);
}

int getIdFromPOSTRequest(const char* request, int requestLength) {
    if (requestLength < 16) {
        return ERROR;
    }
    if (request[14] != '/' || request[16] != '/') {
        return ERROR;
    }
    if (request[15] < '0' || request[15] > '9') {
        return ERROR;
    }
    return request[15] - '0';
}

int getTransactionFromBody(char* request, Transaction* transaction) {
    // Find body in request
    char* body = strstr(request, "{");
    errIfNull(body);

    // Find valor key in body
    char* valor = strstr(body, "valor");
    errIfNull(valor);
    valor = strstr(valor, ":");
    errIfNull(valor);
    valor = &valor[1];
    transaction->valor = atoi(valor);

    // Find tipo key in body
    char* tipo = strstr(body, "tipo");
    errIfNull(tipo);
    tipo = strstr(tipo, ":");
    errIfNull(tipo);
    tipo = strstr(tipo, "\"");
    errIfNull(tipo);
    transaction->tipo = tipo[1];

    // Find descricao key in body
    char* descricaoStart = strstr(body, "descricao");
    errIfNull(descricaoStart);
    descricaoStart = strstr(descricaoStart, ":");
    errIfNull(descricaoStart);
    // Find the quote before the value
    descricaoStart = strstr(descricaoStart, "\"");
    errIfNull(descricaoStart);
    descricaoStart++;
    // Find the quote after the value
    char* descricaoEnd = strstr(descricaoStart, "\"");
    errIfNull(descricaoEnd);
    // Copy the value to the transaction
    char descricao[DESCRIPTION_SIZE];
    int length = descricaoEnd - descricaoStart;
    strncpy(descricao, descricaoStart, length);
    descricao[length] = '\0';
    strcpy(transaction->descricao, descricao);

    // Set the transaction realizada_em to the current time
    getCurrentTimeStr(transaction->realizada_em);

    return SUCCESS;
}

void serializePostResponse(User* user, char* response) {
    const char* postResponseTemplate = "HTTP/1.1 200 OK\nContent-Type: application/json\n\n{\"limite\":%d, \"saldo\":%d}";
    sprintf(response, postResponseTemplate, user->limit, user->total);
}
#endif