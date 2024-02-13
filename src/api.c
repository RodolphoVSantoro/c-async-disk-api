#include "httpHandler.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return ERROR;
    }

    const int SERVER_PORT = atoi(argv[1]);

    int serverSocket = setupServer(SERVER_PORT, SERVER_BACKLOG);
#ifdef LOGGING
    printf("{ Server is running(%d) }\n", serverSocket);
    printf("{ Listening on port %d }\n", SERVER_PORT);
    printf("{ FD_SETSIZE: %d }\n\n", FD_SETSIZE);
#endif

    // Set of socket descriptors
    fd_set currentSockets, readySockets;

    // Initialize the set of active sockets
    FD_ZERO(&currentSockets);
    FD_SET(serverSocket, &currentSockets);

    while (true) {
        readySockets = currentSockets;

        // Wait for an activity on one of the sockets
        if (select(FD_SETSIZE, &readySockets, NULL, NULL, NULL) < 0) {
            printf("Select failed");
            return ERROR;
        }

        // Check all sockets for activity
        for (int socket = 0; socket < FD_SETSIZE; socket++) {
            if (FD_ISSET(socket, &readySockets)) {
                // Accept new connection
                if (socket == serverSocket) {
                    struct sockaddr_in clientAddress;
                    socklen_t clientAddressSize = sizeof(clientAddress);
                    int clientSocket = accept(serverSocket, (SA*)&clientAddress, &clientAddressSize);
                    FD_SET(clientSocket, &currentSockets);
                } else {
                    // Handle client request
                    int clientSocket = socket;
                    char request[SOCKET_READ_SIZE];
                    int bytesRead = recv(clientSocket, request, sizeof(request), SEND_DEFAULT);

                    if (bytesRead >= 1 && bytesRead < SOCKET_READ_SIZE) {
                        request[bytesRead] = '\0';
#ifdef LOGGING
                        int sentResult = handleRequest(request, bytesRead, clientSocket);
                        if (sentResult == ERROR) {
                            printf("{ Error sending response }\n");
                        } else {
                            printf("{ Request handled }\n");
                        }
#else
                        handleRequest(request, bytesRead, clientSocket);
#endif
                    }

                    close(clientSocket);
                    FD_CLR(clientSocket, &currentSockets);
                }
            }
        }
    }

    close(serverSocket);
    return EXIT_SUCCESS;
}
