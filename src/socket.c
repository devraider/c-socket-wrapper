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

Socket *server_accept(ServerSocket *server)
{
    // Allocate memory for a new Socket structure to hold client info
    Socket *client_socket = (Socket *)malloc(sizeof(Socket));
    if (!client_socket)
    {
        perror("[SERVER] malloc failed");
        return NULL;
    }

    // Prepare to accept incoming connection
    socklen_t addr_len = sizeof(client_socket->address);

    /*
     * accept() — what it really does (detailed)
     *
     * 1) Purpose
     *    - accept() retrieves a connection from the pending queue of a listening socket.
     *    - It creates a new socket for the client connection and returns its file descriptor.
     *    - New socket is created because pending queue sockets are not used for data transfer and are just placeholders.
     *
     * 2) Arguments used here
     *    - sockfd: the listening socket file descriptor.
     *    - addr: pointer to a struct sockaddr_in to store client address info.
     *    - addrlen: pointer to socklen_t, which is updated with the size of the client address.
     *
     * 3) What the kernel does on accept
     *    - Removes a connection from the pending queue (if any).
     *    - Creates a new socket for communication with the client.
     *    - Returns a new file descriptor for this new connection.
     *
     * 4) Return value & error handling
     *    - On success, returns a new file descriptor for the accepted connection.
     *    - On error, returns -1 and sets errno (use perror()/strerror(errno)).
     *
     * 5) Common errors & causes
     *    - EBADF: The sockfd is not a valid file descriptor.
     *    - EINVAL: The sockfd is not of type SOCK_STREAM or SOCK_SEQPACKET,
     *      or it has not been bound with bind().
     *
     * 6) Debugging tips
     *    - Ensure listen() was called successfully before accept().
     *
     */
    client_socket->fd = accept(server->server_socket.fd,
                               (struct sockaddr *)&client_socket->address,
                               &addr_len);

    if (client_socket->fd < 0)
    {
        perror("[SERVER] accept failed");
        free(client_socket);
        return NULL;
    }

    // Convert network byte order to host byte order for the port number
    // ntohs(): "network to host short" (short = 16-bit number like port)
    // The port is stored in network byte order (big-endian) in the struct.
    // We convert it to host byte order so we can read/print it as a normal number.
    client_socket->port = ntohs(client_socket->address.sin_port);

    /*
     * inet_ntop() — convert binary IP address to human-readable string format
     *
     * Purpose:
     *   - Takes a binary IP address (32-bit for IPv4) and converts it to
     *     dotted decimal notation (e.g., "192.0.2.1").
     *   - "ntop" = "network to presentation" (binary -> human-readable).
     *
     * Arguments used here:
     *   1) AF_INET: Address family (IPv4 in this case).
     *   2) &client_socket->address.sin_addr: Pointer to the binary IP address
     *      stored in network byte order (big-endian format).
     *      This is a 32-bit unsigned integer holding the IP in binary form.
     *   3) client_socket->ip: Character buffer where the string will be stored.
     *      Must be large enough to hold the result (e.g., "255.255.255.255" = 15 chars + null).
     *   4) sizeof(client_socket->ip): Size of the buffer.
     *      Protects against buffer overflow by telling inet_ntop the max size available.
     *
     * How it works:
     *   - inet_ntop takes the 32-bit binary IP (e.g., 0xC0000201 for 192.0.2.1).
     *   - It converts each octet (8-bit part) to decimal.
     *   - Writes the dotted decimal string to the buffer (e.g., "192.0.2.1").
     *   - Null-terminates the string for safe C string handling.
     *
     * Return value:
     *   - Returns a pointer to the output buffer on success.
     *   - Returns NULL on error (invalid address family or buffer too small).
     *
     * Why we need this:
     *   - The kernel stores IP addresses in binary form for efficient processing.
     *   - Humans need to read/print IP addresses as strings (e.g., in debug output).
     *   - inet_ntop handles the conversion automatically for us.
     *
     * Related functions:
     *   - inet_pton(): The reverse operation (string -> binary).
     *   - We used inet_pton earlier to convert "0.0.0.0" -> binary for bind().
     */
    inet_ntop(AF_INET, &client_socket->address.sin_addr,
              client_socket->ip, sizeof(client_socket->ip));

    printf("[SERVER] Accepted connection from %s:%d (fd: %d)\n",
           client_socket->ip, client_socket->port, client_socket->fd);

    return client_socket;
}

int socket_send(Socket *socket, const char *data)
{

    /*
     * send() - Send data on a socket (detailed kernel-level explanation)
     *
     * Purpose:
     *   - Sends data from the buffer pointed to by 'data' to the socket.
     *   - Returns the number of bytes sent, or -1 on error.
     *
     * Arguments:
     *   1) socket->fd: The file descriptor of the socket (from accept()).
     *   2) data: Pointer to the buffer containing data to send.
     *   3) strlen(data): Length of the data in bytes (assumes null-terminated string).
     *   4) 0: Flags (0 = no special behavior).
     *
     * What happens at the kernel level when send() is called:
     *
     * 1) Data Copying
     *    - The kernel copies the data from user-space memory (your buffer)
     *      into kernel-space memory (the socket send buffer).
     *    - This happens because user programs shouldn't directly access kernel memory.
     *    - The kernel allocates a send buffer (typically a few KB to MB depending on SO_SNDBUF).
     *
     * 2) TCP Stack Processing
     *    - The kernel passes the data to the TCP/IP stack.
     *    - The TCP layer breaks large data into segments (typically ~1460 bytes for Ethernet).
     *    - Each segment gets a TCP header with:
     *      * Source and destination port numbers
     *      * Sequence number (for ordering at receiver)
     *      * Checksum (for error detection)
     *      * Various flags (SYN, ACK, FIN, RST, etc.)
     *
     * 3) IP Layer Processing
     *    - The IP layer adds an IP header with:
     *      * Source and destination IP addresses
     *      * TTL (Time-To-Live, decremented at each router)
     *      * Protocol type (6 for TCP)
     *      * Another checksum for error detection
     *
     * 4) Hardware Layer
     *    - The data packet is passed to the network interface (NIC driver).
     *    - The NIC adds an Ethernet header (MAC addresses, frame type).
     *    - The NIC transmits the frame onto the physical network (WiFi, Ethernet, etc.).
     *    - The frame is broken into bits and sent electronically.
     *
     * 5) Buffering and Flow Control
     *    - If the send buffer is full, send() may block (wait) until space is available.
     *    - TCP implements flow control: the receiver tells the sender how much
     *      data it can accept (via the TCP window field).
     *    - If the remote host's receive buffer is full, send() may block even if
     *      the local send buffer has space.
     *
     * 6) Acknowledgment (ACK) from Receiver
     *    - The remote TCP stack receives the data and sends back an ACK.
     *    - The ACK tells the sender which bytes were received successfully.
     *    - If an ACK doesn't arrive within a timeout, the kernel retransmits the data.
     *    - This ensures reliable delivery (TCP's job).
     *
     * 7) Return Value Meaning
     *    - Returns the number of bytes the kernel accepted (buffered/sent).
     *    - This does NOT mean the remote host received the data yet.
     *    - send() returning 10 means: "I buffered 10 bytes for transmission."
     *    - The actual network transmission happens asynchronously.
     *
     * 8) Common Errors & Causes
     *    - EBADF: Invalid file descriptor.
     *    - EPIPE: Connection closed by remote host (broken pipe).
     *    - ECONNRESET: Connection reset by peer (remote host crashed or reset).
     *    - EAGAIN/EWOULDBLOCK: Send buffer full (non-blocking socket).
     *
     * 9) Important Notes
     *    - send() is asynchronous: data goes to the kernel buffer, not directly on the wire.
     *    - The kernel handles retransmission, ordering, and error checking automatically.
     *    - Your application doesn't wait for network packets; it just hands off the data.
     *    - Network delays (latency) are hidden from the application.
     *
     * 10) Debugging Tips
     *    - Use `ss -tnp` or `netstat -anp` to see socket send buffer sizes.
     *    - Packet sniffers (tcpdump, Wireshark) show the actual bytes on the wire.
     *    - Use SO_SNDBUF socket option to tune the send buffer size.
     */
    int bytes_sent = send(socket->fd, data, strlen(data), 0);

    if (bytes_sent < 0)
    {
        perror("send failed");
        return -1;
    }

    printf("[SEND] Sent %d bytes: %s\n", bytes_sent, data);
    return bytes_sent;
}

int socket_receive(Socket *socket, char *buffer, int buffer_size)
{
    /*
     * recv() - Receive data on a socket (detailed kernel-level explanation)
     *
     * Purpose:
     *   - Retrieves data received on the socket from the kernel receive buffer.
     *   - Copies data from kernel-space to user-space (your buffer).
     *   - Returns the number of bytes received, 0 if connection closed, -1 on error.
     *
     * Arguments:
     *   1) socket->fd: The file descriptor of the socket (from accept()).
     *   2) buffer: Pointer to user-space memory where received data will be stored.
     *   3) buffer_size - 1: Maximum number of bytes to read (leaving room for null terminator).
     *   4) 0: Flags (0 = blocking, standard behavior).
     *
     * What happens at the kernel level when recv() is called:
     *
     * 1) Blocking Behavior (Default)
     *    - If the receive buffer is empty (no data arrived yet), recv() blocks (sleeps).
     *    - The kernel puts your process in a "wait queue" for this socket.
     *    - When data arrives, the kernel wakes up your process.
     *    - recv() returns immediately if data is already buffered.
     *
     * 2) Data Arrival from Network
     *    - Network packets arrive at the NIC (network interface card).
     *    - NIC driver extracts the Ethernet frame and passes it to the kernel.
     *    - Kernel's network stack (IP layer) processes the IP header:
     *      * Validates checksum
     *      * Checks destination IP matches local machine
     *      * Checks TTL (rejects if expired)
     *    - Kernel passes packet to TCP layer.
     *
     * 3) TCP Processing at Kernel
     *    - TCP layer validates the TCP header and checksum.
     *    - Checks sequence number (ensures ordered delivery).
     *    - If out-of-order, stores in reorder buffer until missing segments arrive.
     *    - Extracts payload data and places it in the receive buffer for your socket.
     *    - Sends ACK back to sender (automatic, transparent to your app).
     *
     * 4) Kernel Buffer Management
     *    - The kernel maintains a receive buffer (typically a few KB to MB).
     *    - Data sits here until your application calls recv().
     *    - If the buffer fills up, kernel sends a TCP window size = 0 to stop sender.
     *    - The sender then pauses transmission until you drain the buffer.
     *
     * 5) Data Copying to User Space
     *    - When recv() is called, kernel copies data from the receive buffer to your buffer.
     *    - Only the amount specified by (buffer_size - 1) is copied.
     *    - We subtract 1 to leave room for null terminator (C string convention).
     *    - This copying is necessary for security (isolate user programs from kernel).
     *
     * 6) Return Value Meaning
     *    - Returns number of bytes copied into your buffer.
     *    - Returns 0 if the remote host closed the connection gracefully (FIN received).
     *    - Returns -1 on error (check errno for details).
     *    - Note: recv() is partial read; it may return fewer bytes than you requested.
     *      For example, recv(fd, buf, 1000, 0) might return only 50 bytes even if
     *      1000 bytes are available. You must call recv() in a loop to get all data.
     *
     * 7) Error Handling & Connection States
     *    - EBADF: Invalid file descriptor.
     *    - ECONNRESET: Remote host reset connection (sent RST).
     *    - ETIMEDOUT: No data received for a long time (connection stale).
     *    - 0 return: Remote closed gracefully (FIN flag received).
     *
     * 8) Partial Reads (Important!)
     *    - recv() may return fewer bytes than requested.
     *    - The kernel returns as soon as there's some data available.
     *    - Application must loop and call recv() again until all expected data arrives.
     *    - This is normal TCP behavior; you can't rely on recv() returning all bytes at once.
     *
     * 9) ACK Behavior (Automatic)
     *    - The kernel automatically sends TCP ACK for received data.
     *    - This happens even before your application calls recv().
     *    - ACK tells the sender the data arrived safely at this machine (not yet at the app).
     *
     * 10) Debugging Tips
     *    - Use `ss -tnp` to see receive buffer usage and connection state.
     *    - tcpdump or Wireshark shows actual packets arriving on the NIC.
     *    - Add recv() in a loop to handle partial reads properly.
     *    - Use MSG_DONTWAIT flag for non-blocking recv() if needed.
     */
    int bytes_received = recv(socket->fd, buffer, buffer_size - 1, 0);

    if (bytes_received < 0)
    {
        perror("recv failed");
        return -1;
    }

    // Null-terminate the received data (make it a valid C string)
    buffer[bytes_received] = '\0';

    printf("[RECEIVE] Received %d bytes: %s\n", bytes_received, buffer);
    return bytes_received;
}

int socket_close(Socket *socket)
{
    if (socket && socket->fd >= 0)
    {
        printf("[CLOSE] Closing socket (fd: %d)\n", socket->fd);

        /*
         * close() - Close a socket (detailed kernel-level explanation)
         *
         * Purpose:
         *   - Closes the socket file descriptor, releasing associated resources.
         *   - Notifies the kernel that the socket is no longer needed.
         *
         * What happens at the kernel level when close() is called:
         *
         * 1) Resource Cleanup
         *    - The kernel marks the file descriptor as closed.
         *    - Releases memory and buffers associated with the socket.
         *    - Decrements reference counts for underlying structures.
         *
         * 2) TCP Connection Teardown
         *    - If it's a TCP socket, the kernel initiates connection termination.
         *    - Sends a FIN packet to the remote host to signal connection closure.
         *    - Waits for an ACK from the remote host confirming receipt of FIN.
         *    - Enters TIME_WAIT state to handle any delayed packets (prevents confusion).
         *
         * 3) File Descriptor Reuse
         *    - The file descriptor number becomes available for reuse by future socket() calls.
         *    - Subsequent calls to socket() may return the same fd number.
         *
         * 4) Return Value & Error Handling
         *    - Returns 0 on success, -1 on error and sets errno.
         *    - Common errors include EBADF (invalid fd) if already closed.
         *
         * 5) Important Notes
         *    - Always call close() to avoid resource leaks.
         *    - Failing to close sockets can exhaust system file descriptors.
         *    - After close(), the socket fd should not be used again.
         *
         * 6) Debugging Tips
         *    - Use `lsof` or `ss` to check open sockets before and after close().
         *    - Monitor system resource usage to detect leaks.
         *
         */
        int close_result = close(socket->fd);
        if (close_result < 0)
        {
            perror("close failed");
            return -1;
        }
        socket->fd = -1;
    }
    return 0;
}

void server_free(ServerSocket *server)
{
    if (server)
    {
        socket_close(&server->server_socket);

        /*
         * free() - Free a ServerSocket structure
         * Purpose:
         *   - Releases memory allocated for the ServerSocket structure.
         *   - Ensures proper cleanup of resources.
         *
         * What happens at the kernel level when close() is called:
         *
         * 1) Memory Deallocation
         *   - The malloc()ed memory for the ServerSocket structure is returned to the heap
         */
        free(server);
    }
}