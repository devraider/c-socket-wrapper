#include "socket.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s server <ip> <port>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0)
    {
        if (argc < 4)
        {
            fprintf(stderr, "Usage: %s server <ip> <port>\n", argv[0]);
            return 1;
        }

        char *ip = argv[2];
        int port = atoi(argv[3]);

        ServerSocket *server = create_server_socket(ip, port, 5);

        if (!server)
        {
            fprintf(stderr, "Failed to create server\n");
            return 1;
        }

        server_bind(server);
        server_listen(server);

        while (1)
        {
            Socket *client = server_accept(server);
            if (!client)
            {
                fprintf(stderr, "Failed to accept client\n");
                continue;
            }
            socket_send(client, "Welcome to the server!\n");

            char buffer[SOCKET_BUFFER_SIZE];
            int bytes_received = socket_receive(client, buffer, SOCKET_BUFFER_SIZE - 1);
            if (!bytes_received)
            {
                fprintf(stderr, "Failed to receive data from client\n");
                socket_close(client);
                free(client);
                continue;
            }

            socket_send(client, "Message received\n");

            socket_close(client);
            free(client);
        }
    }
    else
    {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}