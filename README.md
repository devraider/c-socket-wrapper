# C Socket Programming Learning Project

A C socket programming tutorial and implementation with detailed educational comments explaining TCP/IP networking at the kernel level.

## Overview

This project demonstrates **TCP socket programming** using POSIX socket APIs. It implements a working server-client architecture with extensive kernel-level explanations in the code comments. Perfect for learning how:

- Sockets are created, bound, and connected
- Data is transmitted and received
- The kernel handles networking operations
- TCP/IP stack processes packets

## Project Structure

```
.
├── README.md                 # This file
├── Makefile                  # Build automation
├── src/
│   ├── main.c              # Server/client application entry point
│   ├── socket.h            # Socket wrapper library header
│   └── socket.c            # Socket wrapper library implementation
└── build/                  # Compiled binaries (created by make)
    └── socket_discovery    # Executable
```

## Building

Compile the project using the Makefile:

```bash
make build      # Compile the project
make clean      # Remove build artifacts
make run        # Run the executable (see Usage section)
```

### Build Details

- **Compiler**: GCC with flags `-Wall -Wextra -std=c11 -g`
- **Language**: C11 standard
- **Platform**: POSIX (Linux, macOS)
- **Output**: `build/socket_discovery` executable

## Usage

### Starting a Server

```bash
make run ARGS="server <IP> <PORT>"
```

**Examples:**

```bash
# Listen on all interfaces
make run ARGS="server 0.0.0.0 8000"

# Listen on localhost only
make run ARGS="server 127.0.0.1 8000"

# Listen on a specific interface IP
make run ARGS="server 192.168.1.100 8000"
```

### Connecting a Client

```bash
make run ARGS="client <HOSTNAME_OR_IP> <PORT>"
```

**Examples:**

```bash
# Connect to local server
make run ARGS="client localhost 8000"

# Connect to server on another machine
make run ARGS="client 192.168.1.100 8000"

# Connect using hostname
make run ARGS="client example.com 8000"
```

## Architecture

### Socket Wrapper Library (`socket.h` / `socket.c`)

The project provides a high-level wrapper around raw socket APIs:

#### Data Structures

**`Socket`** - Represents a single socket connection

```c
typedef struct {
    int fd;                           // File descriptor
    struct sockaddr_in address;       // IP:port information
    int port;                         // Port number (host byte order)
    char ip[INET_ADDRSTRLEN];        // IP address string
} Socket;
```

**`ServerSocket`** - Server-specific socket wrapper

```c
typedef struct {
    Socket server_socket;    // Base socket structure
    int backlog;             // Pending connection queue size
} ServerSocket;
```

**`ClientSocket`** - Client-specific socket wrapper

```c
typedef struct {
    Socket client_socket;           // Base socket structure
    struct sockaddr_in server_addr; // Server address for connection
} ClientSocket;
```

#### Core Functions

| Function                                  | Purpose                              |
| ----------------------------------------- | ------------------------------------ |
| `create_server_socket(ip, port, backlog)` | Create and configure a server socket |
| `server_bind(server)`                     | Bind socket to IP:port               |
| `server_listen(server)`                   | Mark socket as accepting connections |
| `server_accept(server)`                   | Accept incoming client connection    |
| `socket_send(socket, data)`               | Send data on socket                  |
| `socket_receive(socket, buffer, size)`    | Receive data from socket             |
| `socket_close(socket)`                    | Close socket and free resources      |

## Learning Topics Covered

### 1. **Socket Creation** (`socket()`)

- Creating AF_INET (IPv4) TCP (SOCK_STREAM) sockets
- Understanding file descriptors
- Memory allocation for socket structures

### 2. **Address Binding** (`bind()`)

- Binding socket to IP:port
- sockaddr_in structure initialization
- Wildcard binding (0.0.0.0) vs specific IP
- TIME_WAIT and SO_REUSEADDR socket options
- Common binding errors (EADDRINUSE, EACCES, EADDRNOTAVAIL)

### 3. **Listening for Connections** (`listen()`)

- Marking socket as passive (accepting)
- Backlog queue and pending connections
- Kernel resource allocation
- Error handling

### 4. **Accepting Connections** (`accept()`)

- Creating new socket for each client
- Extracting client address information
- Binary IP address conversion with `inet_ntop()`
- Port conversion with `ntohs()`

### 5. **Address Conversion**

- **`inet_pton()`**: String IP → binary format (used before `bind()`)
- **`inet_ntop()`**: Binary IP → human-readable string (for display)
- **`htons()`**: Host → network byte order (ports)
- **`ntohs()`**: Network → host byte order (ports)
- **`gethostbyname()`**: Hostname resolution

### 6. **Data Transmission** (`send()`)

- Sending data through kernel socket buffer
- TCP segmentation and header construction
- Flow control and acknowledgments
- Understanding partial sends
- Kernel buffering behavior

### 7. **Data Reception** (`recv()`)

- Receiving data from kernel receive buffer
- Blocking behavior and wait queues
- Partial reads and loop handling
- Connection state detection (FIN, RST)
- Automatic ACK behavior

### 8. **Kernel-Level Networking**

Each major function includes detailed comments explaining:

- User-space to kernel-space data copying
- TCP/IP stack processing
- NIC (network interface card) operations
- Packet structure and headers
- Retransmission and error recovery
- Flow control mechanisms

## Key Concepts Explained

### Byte Order (Endianness)

- **Host byte order**: How your CPU stores numbers (usually little-endian on x86/ARM)
- **Network byte order**: Big-endian standard used by TCP/IP
- Functions like `htons()` and `ntohs()` convert between them
- Always use conversion functions for ports and IP addresses!

### File Descriptors

- Returned by `socket()`, each one represents a unique socket
- Used like file handles: 0=stdin, 1=stdout, 2=stderr, 3+=your sockets
- Passed to `send()`, `recv()`, `bind()`, etc.

### Blocking vs Non-Blocking

- **Blocking (default)**: `recv()` waits until data arrives; `listen()` waits for connections
- **Non-Blocking**: Operations return immediately with EAGAIN/EWOULDBLOCK
- This project uses blocking mode for simplicity

### TCP Connection States

- **LISTEN**: Server waiting for connections
- **SYN_SENT**: Client waiting for server ACK
- **ESTABLISHED**: Connected, data transfer possible
- **FIN_WAIT**: Waiting for connection close
- **TIME_WAIT**: Recently closed, kernel prevents port reuse

## Example Session

**Terminal 1 (Server):**

```bash
$ make run ARGS="server 127.0.0.1 8000"
[SERVER] Socket created successfully (fd: 3)
[SERVER] Binding socket to 127.0.0.1:8000...
[SERVER] Socket bound successfully
[SERVER] Listening on 127.0.0.1:8000 (backlog: 10)
[SERVER] Waiting for connections...
[SERVER] Accepted connection from 127.0.0.1:53241 (fd: 4)
[RECEIVE] Received 13 bytes: Hello, Server!
[SEND] Sent 13 bytes: Hello, Client!
```

**Terminal 2 (Client):**

```bash
$ make run ARGS="client localhost 8000"
[CLIENT] Connecting to localhost:8000...
[CLIENT] Connected successfully!
[SEND] Sent 13 bytes: Hello, Server!
[RECEIVE] Received 13 bytes: Hello, Client!
```

## Common Errors & Solutions

| Error          | Cause                    | Solution                                           |
| -------------- | ------------------------ | -------------------------------------------------- |
| `EADDRINUSE`   | Port already in use      | Wait for TIME_WAIT to expire or use `SO_REUSEADDR` |
| `ECONNREFUSED` | Server not listening     | Start server first                                 |
| `ETIMEDOUT`    | Connection stalled       | Check network/firewall, increase timeout           |
| `EPIPE`        | Remote closed connection | Handle gracefully in your application              |
| `EACCES`       | Permission denied        | Ports < 1024 need root; use port > 1024            |

## Debugging Tips

### View Active Sockets

```bash
# macOS/Linux
ss -tnp              # Show TCP sockets with process info
netstat -anv         # Alternative view
lsof -iTCP           # List open TCP sockets

# Filter by port
ss -tnp | grep 8000  # Show connections on port 8000
```

### Monitor Network Traffic

```bash
# macOS
tcpdump -i en0 port 8000    # Capture on en0 interface

# Linux
tcpdump -i eth0 port 8000   # Capture on eth0 interface
```

### Check Socket Buffer Sizes

```bash
# macOS
sysctl net.inet.tcp.sendspace
sysctl net.inet.tcp.recvspace

# Linux
cat /proc/sys/net/ipv4/tcp_rmem
cat /proc/sys/net/ipv4/tcp_wmem
```

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    User Application                         │
│  (main.c: calls socket_send(), socket_receive())            │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│          Socket Wrapper Library (socket.c)                  │
│  - create_server_socket()      - socket_send()              │
│  - server_bind()               - socket_receive()           │
│  - server_listen()             - socket_close()             │
│  - server_accept()                                          │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│          POSIX Socket API (system calls)                    │
│  socket() → bind() → listen() → accept() → send()/recv()    │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────┐
│              Kernel Network Stack                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ TCP Layer (sequencing, reliability, flow control)    │   │
│  │ ┌────────────────────────────────────────────────┐   │   │
│  │ │ IP Layer (routing, addressing)                 │   │   │
│  │ │ ┌──────────────────────────────────────────┐   │   │   │
│  │ │ │ Link Layer (Ethernet, MAC addresses)     │   │   │   │
│  │ │ │ ┌──────────────────────────────────────┐ │   │   │   │
│  │ │ │ │ NIC Driver (hardware transmission)   │ │   │   │   │
│  │ │ │ └──────────────────────────────────────┘ │   │   │   │
│  │ │ └──────────────────────────────────────────┘   │   │   │
│  │ └────────────────────────────────────────────────┘   │   │
│  └──────────────────────────────────────────────────────┘   │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ▼
          ┌──────────────┐
          │  Network     │
          │  (Ethernet,  │
          │   WiFi, etc) │
          └──────────────┘
```

## Socket Lifecycle

```
SERVER                              CLIENT
  ▼                                   ▼
socket()  ──────────────────────  socket()
  ▼                                   ▼
bind()                            connect()
  ▼                                   ▼
listen()                              │
  ▼                                   │
accept() ◄────── TCP 3-Way ───────────┤
  ▼              Handshake            ▼
  │        (SYN, SYN-ACK, ACK)        │
  │                                   │
  ├─────── ESTABLISHED ───────────────┤
  │                                   │
  ├──── send() ────────────────► recv()
  │                                   │
  ├──── recv() ◄──────── send() ──────┤
  │                                   │
  ├─────── close() ───────────────────┤
  ▼                                   ▼
```

## Compiler Warnings

The project compiles with `-Wall -Wextra` flags to catch common mistakes. All warnings are treated seriously:

- Unused variables
- Missing return values
- Implicit type conversions
- Uninitialized variables

## Further Learning

### Topics to Explore

1. **Non-blocking sockets**: Use `MSG_DONTWAIT` flag or `O_NONBLOCK`
2. **Multiplexing**: `select()`, `poll()`, `epoll()` for handling multiple connections
3. **IPv6**: AF_INET6 and `struct sockaddr_in6`
4. **UDP**: SOCK_DGRAM for connectionless communication
5. **Signal handling**: Handle SIGPIPE, SIGCHLD for graceful shutdown
6. **Threading**: Use pthreads for concurrent client handling
7. **TLS/SSL**: Encrypt communication with OpenSSL

### System Calls to Study

- `socket()`, `bind()`, `listen()`, `accept()`, `connect()`
- `send()`, `recv()`, `sendto()`, `recvfrom()`
- `shutdown()`, `close()`
- `setsockopt()`, `getsockopt()`
- `select()`, `poll()`, `epoll()`
- `fork()`, `exec()` for process management

## References

- **POSIX Socket API**: `man socket`, `man send`, `man recv` (in terminal)
- **TCP/IP Stack**: RFC 793 (TCP), RFC 791 (IP)
- **Byte Order**: Always use `htons()`, `ntohs()` for network byte order
- **Debugging**: tcpdump, Wireshark, ss, netstat

## License

Educational project for learning C socket programming during personal study of Computer Science: A Programmer's Perspective (3rd Edition) by Randal E. Bryant and David R. O'Hallaron.

## Author Notes

Every major socket operation includes detailed kernel-level explanations to help you understand:

- What the operating system does behind the scenes
- How data flows through the TCP/IP stack
- Why certain functions and options exist
- Common pitfalls and how to avoid them

Read the comments in `socket.c` carefully—they're the heart of this learning resource!
