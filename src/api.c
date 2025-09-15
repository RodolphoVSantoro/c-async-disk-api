#include "httpHandler.h"

int serverSocket;
int epollFileDescriptor;

// For profiling even if the server closes from a ctrl+c signal
void signal_callback_handler(int signum) {
    printf("{ Caught signal %d, shutting down }\n", signum);
    close(epollFileDescriptor);
    close(serverSocket);
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return ERROR;
    }

    const int SERVER_PORT = atoi(argv[1]);

    log("{ Starting server... }\n");
#ifdef RESET_DB
    int createFolderResult = system("mkdir -p data");
    raiseIfNotSuccess(createFolderResult, "Failed to create data folder");
    int resetDbResult = initDb();
    raiseIfNotSuccess(resetDbResult, "Failed to reset database");
#endif

    serverSocket = setupServer(SERVER_PORT, SERVER_BACKLOG);

    signal(SIGINT, signal_callback_handler);
    signal(SIGTERM, signal_callback_handler);

    log("{ Server is running(%d) }\n", serverSocket);
    log("{ Listening on port %d }\n", SERVER_PORT);

    epollFileDescriptor = epoll_create(1);
    raiseIfError(epollFileDescriptor, "Failed to create epoll file descriptor");
    struct epoll_event server_event;
    
    server_event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	server_event.data.fd = serverSocket;
    int addServerPollResult = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, serverSocket, &server_event);
    raiseIfError(addServerPollResult, "Failed to add server socket to epoll");

    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int INF_TIMEOUT = -1;
        log("{ Waiting for events... }\n");
        int event_count = epoll_wait(epollFileDescriptor, events, MAX_EVENTS, INF_TIMEOUT);
        if(event_count != 0){
            log("{ %d events }\n", event_count);
        } else{
            log("{ epoll_wait timed out }\n");
        }
        for(int i = 0; i < event_count; i++){
            log("{ Handling event %d }\n", i);
            int socket = events[i].data.fd;
            if(socket == serverSocket){
                struct sockaddr_in clientAddress;
                socklen_t clientAddressSize = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (SA*)&clientAddress, &clientAddressSize);
                raiseIfError(clientSocket, "Failed to accept connection");

                struct epoll_event client_event;
                client_event.events = EPOLLIN | EPOLLET;
                client_event.data.fd = clientSocket;
                int addClientResult = epoll_ctl(epollFileDescriptor, EPOLL_CTL_ADD, clientSocket, &client_event);
                raiseIfError(addClientResult, "Failed to add client socket to epoll");
                log("{ Accepted connection %d }\n", clientSocket);
            }
            else if(events[i].events & EPOLLIN){
                char request[SOCKET_READ_SIZE];
                int bytesRead = recv(socket, request, sizeof(request), SEND_DEFAULT);

                if (bytesRead >= 1 && bytesRead < SOCKET_READ_SIZE) {
                    request[bytesRead] = '\0';
                    int sentResult = handleRequest(request, bytesRead, socket);
                    if (sentResult == ERROR) {
                        log("{ Error sending response }\n");
                    } else {
                        log("{ Request handled }\n");
                    }
                }

                int closeResult = close(socket);
                raiseIfError(closeResult, "Failed to close client socket");
            }
        }
    }


    if (close(epollFileDescriptor)) {
		fprintf(stderr, "Failed to close epoll file descriptor\n");
		return 1;
	}

    close(serverSocket);
    return EXIT_SUCCESS;
}
