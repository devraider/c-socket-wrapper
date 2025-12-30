#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

ServerSocket *create_server_socket(char *ip, int port, int backlog)
{
    // malloc: Allocate memory dynamically for a ServerSocket structure
    // sizeof(ServerSocket): Calculate how many bytes we need
    // (ServerSocket *): Cast the returned void pointer to ServerSocket pointer
    // This creates space to store the server data
    ServerSocket *server = (ServerSocket *)malloc(sizeof(ServerSocket));

    // Check if socket() failed (returns -1 on error)
    if (!server)
    {
        perror("Couln't allocate memory for server socket");
        return NULL;
    }

    // socket(): Create a socket - returns a file descriptor (a number like 3, 4, 5...)
    // AF_INET: Address Family - IPv4 (not IPv6, not other protocols)
    // SOCK_STREAM: Socket type - TCP (reliable, ordered delivery)
    // 0: Protocol - use default protocol for AF_INET + SOCK_STREAM
    // Result is stored in server->server_socket.fd
    server->server_socket.fd = socket(AF_INET, SOCK_STREAM, 0);

    // Check if socket() failed (returns -1 on error)
    if (server->server_socket.fd < 0)
    {
        perror("socket creation failed"); // Print system error
        free(server);                     // Free memory we allocated
        return NULL;                      // Return NULL to indicate failure
    }

    // Store the port number in the server structure for later reference
    server->server_socket.port = port;

    // Store the backlog (queue size for pending connections)
    server->backlog = backlog;

    // Store the IP address in the server structure for later reference
    strcpy(server->server_socket.ip, ip);

    // memset(): Fill memory with zeros (initialize the address structure)
    // &server->server_socket.address: Address of the struct to initialize
    // 0: Fill with zero bytes
    // sizeof(...): How many bytes to fill
    // This clears any garbage data from memory
    memset(&server->server_socket.address, 0, sizeof(server->server_socket.address));

    // sin_family: Set the address family to AF_INET (IPv4)
    // Must match the AF_INET we used in socket() call
    server->server_socket.address.sin_family = AF_INET;

    // htons(): Convert host byte order to network byte order
    // Computers store numbers differently (endianness), network uses big-endian
    // htons = "host to network short" (short = 16-bit number like port)
    // Example: port 5000 -> network representation
    server->server_socket.address.sin_port = htons(port);

    // INADDR_ANY was hardcoded to listen on all IPs
    // Now we convert the IP string we stored in line 41 to binary format
    // inet_pton(): Convert IP string (dotted decimal) to binary format
    // AF_INET: IPv4 format
    // server->server_socket.ip: The IP string we stored ("0.0.0.0" or any other IP)
    // &server->server_socket.address.sin_addr: Where to store the binary IP
    // This allows the server to listen only on the specified IP address
    inet_pton(AF_INET, server->server_socket.ip, &server->server_socket.address.sin_addr);

    // Print success message showing the file descriptor number
    // File descriptors are usually: 0=stdin, 1=stdout, 2=stderr, 3+=our sockets
    printf("[SERVER] Socket created successfully (fd: %d)\n", server->server_socket.fd);

    // Return pointer to the initialized server structure
    return server;
}

int server_bind(ServerSocket *server)
{
    printf("[SERVER] Binding socket to %s:%d...\n", server->server_socket.ip, server->server_socket.port);

    /*
     * bind() — what it really does (detailed)
     *
     * 1) Purpose
     *    - bind(fd, addr, addrlen) tells the kernel: "Deliver packets
     *      addressed to this local IP and port to this socket file descriptor."
     *    - After bind succeeds, the socket is "named" at the OS level.
     *
     * 2) Arguments used here
     *    - fd: the socket() file descriptor (an integer handle).
     *    - (struct sockaddr *)&addr: we build a concrete struct sockaddr_in
     *      (IPv4) and cast it to struct sockaddr* because the sockets API
     *      is protocol-neutral (bind accepts sockaddr for IPv4/IPv6).
     *    - sizeof(addr): size of the concrete structure (use sizeof(struct sockaddr_in)
     *      or sizeof(server->server_socket.address)). This tells the kernel
     *      how many bytes to read from the pointer.
     *
     * 3) What the kernel does on bind
     *    - Validates the address (is the IP assigned to a local interface?).
     *    - Associates the IP:port tuple with the socket.
     *    - If the socket will accept connections, the kernel places incoming
     *      SYNs into the listen/accept queues once listen() is called.
     *
     * 4) Wildcard vs specific IP
     *    - Binding to INADDR_ANY / "0.0.0.0" means accept traffic on any local interface.
     *    - Binding to a specific IP (e.g., "192.0.2.10") restricts the socket to that interface.
     *    - Use inet_pton beforehand to convert string -> binary (sin_addr).
     *
     * 5) Differences from listen/accept
     *    - bind just names the socket. listen marks it passive and creates the
     *      pending-connection queue. accept returns a new FD for each accepted client.
     *
     * 6) Return value & error handling
     *    - bind returns 0 on success, -1 on error and sets errno.
     *    - Use perror()/strerror(errno) to display the failure reason.
     *
     * 7) Common errors & causes
     *    - EADDRINUSE: Port already in use by another socket (or a previous socket in TIME_WAIT).
     *    - EACCES: Permission denied (binding to ports < 1024 usually requires root).
     *    - EADDRNOTAVAIL: The IP you requested isn't assigned to any local interface.
     *
     * 8) TIME_WAIT and SO_REUSEADDR
     *    - A TCP socket that recently closed may leave the port in TIME_WAIT.
     *      Rapid restarts can hit EADDRINUSE even though no process currently "owns" the port.
     *    - setsockopt(SO_REUSEADDR) before bind can allow reusing the address in many dev scenarios,
     *      but it does not magically let two processes listen on the same IP:port simultaneously.
     *    - On some systems you may also see SO_REUSEPORT (different semantics).
     *
     * 9) Debugging tips
     *    - Use `ss -ltnp` or `netstat -anv | grep <port>` to see which process (if any) has the port.
     *    - Print errno with perror() to see the specific error message.
     *
     * 10) Example (call setsockopt BEFORE bind if you want reuse behavior):
     *    int yes = 1;
     *    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
     *
     * 11) Summary
     *    - bind() tells the OS which local address/port your socket will use.
     *    - After bind + listen, the socket can accept incoming connections for that address.
     */
    int bind_result = bind(server->server_socket.fd,
                           (struct sockaddr *)&server->server_socket.address,
                           sizeof(server->server_socket.address));

    if (bind_result < 0)
    {
        perror("[SERVER] Bind failed");
        return -1;
    }

    printf("[SERVER] Socket bound successfully\n");
    return 0;
}

int server_listen(ServerSocket *server)
{
    /*
     * listen() — what it really does (detailed)
     *
     * 1) Purpose
     *    - listen(fd, backlog) tells the kernel: "This socket will accept incoming
     *      connection requests. Please queue them up to 'backlog' length."
     *    - It marks the socket as a passive socket that will be used to accept connections.
     *
     * 2) Arguments used here
     *    - fd: the socket() file descriptor (an integer handle).
     *    - backlog: maximum number of pending connections to queue.
     *      If more connections arrive, they may be refused or ignored.
     *
     * 3) What the kernel does on listen
     *    - Allocates resources for the pending connection queue.
     *    - Prepares to handle incoming SYN packets for TCP connections.
     *    - Incoming connection requests are placed in the queue until accept() is called.
     *
     * 4) Backlog behavior
     *    - The backlog parameter is a hint to the kernel about how many connections
     *      to queue. The actual limit may be higher or lower depending on system settings.
     *    - If the queue is full, new connection attempts may be refused (clients get ECONNREFUSED).
     *
     * 5) Differences from bind/accept
     *    - bind names the socket with an IP:port. listen marks it as ready to accept connections.
     *    - accept retrieves and removes a connection from the pending queue, returning a new FD.
     *
     * 6) Return value & error handling
     *    - listen returns 0 on success, -1 on error and sets errno.
     *    - Use perror()/strerror(errno) to display the failure reason.
     *
     * 7) Common errors & causes
     *    - EBADF: The fd is not a valid file descriptor.
     *    - EINVAL: The socket is not of type SOCK_STREAM or SOCK_SEQPACKET,
     *      or it has not been bound with bind().
     *
     * 8) Debugging tips
     *    - Ensure bind() was called successfully before listen().
     *    - Print errno with perror() to see the specific error message.
     *
     * 9) Summary
     *    - listen() prepares a bound socket to accept incoming connection requests.
     *    - After listen, the socket can queue incoming connections until accept() is called.
     *
     */
    int listen_result = listen(server->server_socket.fd, server->backlog);

    if (listen_result < 0)
    {
        perror("[SERVER] Listen failed");
        return -1;
    }

    printf("[SERVER] Listening on %s:%d (backlog: %d)\n",
           server->server_socket.ip,
           server->server_socket.port,
           server->backlog);
    return 0;
}