#ifndef SOCKET_H
#define SOCKET_H

#include <netinet/in.h>

int SOCKET_BUFFER_SIZE = 1024;

typedef struct
{
    int fd;                     // Socket file descriptor
    struct sockaddr_in address; // Socket address structure
    int port;                   // Port number
    char ip[16];                // IP address (e.g., "127.0.0.1")
} Socket;

typedef struct
{
    Socket server_socket;
    int backlog; // Queue length for pending connections
} ServerSocket;

/* Function prototypes for the socket wrapper library */
ServerSocket *create_server_socket(char *ip, int port, int backlog);
int server_bind(ServerSocket *server);
int server_listen(ServerSocket *server);
Socket *server_accept(ServerSocket *server);

// Send/Receive functions
int socket_send(Socket *socket, const char *data);
int socket_receive(Socket *socket, char *buffer, int buffer_size);

// Cleanup functions
void server_free(ServerSocket *server);
int socket_close(Socket *socket);

#endif