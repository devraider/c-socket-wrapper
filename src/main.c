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
    }
    else
    {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}