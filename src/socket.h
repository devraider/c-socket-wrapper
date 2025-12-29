#include <netinet/in.h>


#ifndef SOCKET_H
#define SOCKET_H

typedef struct {
    int fd;                           // Socket file descriptor
    struct sockaddr_in address;       // Socket address structure
    int port;                         // Port number
    char ip[16];                      // IP address (e.g., "127.0.0.1")
} Socket;

typedef struct {
    Socket server_socket;
    int backlog;                      // Queue length for pending connections
} ServerSocket;
#endif