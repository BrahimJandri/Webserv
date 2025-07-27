# Complex Concepts Deep Dive

## 1. acceptNewConnection() Function Analysis

```cpp
void Server::acceptNewConnection(int server_fd)
{
    struct sockaddr_in client_addr;           // Line 1
    socklen_t client_len = sizeof(client_addr); // Line 2

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len); // Line 3
    
    if (client_fd == -1) // Line 4
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) // Line 5
            return; // No more connections to accept
        perror("accept");
        return;
    }

    // Set non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0); // Line 6
    if (flags == -1 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) // Line 7
    {
        perror("fcntl");
        close(client_fd);
        return;
    }

    // Add client to epoll
    struct epoll_event event; // Line 8
    event.events = EPOLLIN | EPOLLET; // Line 9
    event.data.fd = client_fd; // Line 10

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) // Line 11
    {
        perror("epoll_ctl: client_fd");
        close(client_fd);
        return;
    }

    // Store client
    clients[client_fd] = new Client(client_fd); // Line 12
    clientToServergMap[client_fd] = serverConfigMap[server_fd][0]; // Line 13
}
```

### Line-by-Line Breakdown:

#### Lines 1-2: Client Address Structure
```cpp
struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);
```
**Purpose**: Prepare to receive client's network information
- `sockaddr_in`: Structure containing IPv4 address and port
- `client_len`: Size of the structure (passed by reference to accept())

#### Line 3: accept() System Call
```cpp
int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
```
**Purpose**: Accept a pending connection from the server socket's queue

**What happens internally**:
```
Before accept():
┌─────────────────┐    ┌─────────────────┐
│  Server Socket  │    │  Connection     │
│    (listening)  │    │     Queue       │
│     fd: 3       │    │  [Client A]     │
└─────────────────┘    │  [Client B]     │
                       │  [Client C]     │
                       └─────────────────┘

After accept():
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Server Socket  │    │  Connection     │    │  Client Socket  │
│    (listening)  │    │     Queue       │    │   (connected)   │
│     fd: 3       │    │  [Client B]     │    │     fd: 6       │
└─────────────────┘    │  [Client C]     │    │   Client A      │
                       └─────────────────┘    └─────────────────┘
```

**Return Values**:
- **Success**: New file descriptor for client socket
- **Error**: -1 (check errno)
- **No connections**: -1 with errno = EAGAIN/EWOULDBLOCK

#### Lines 4-5: Error Handling
```cpp
if (errno == EAGAIN || errno == EWOULDBLOCK)
    return; // No more connections to accept
```
**EAGAIN/EWOULDBLOCK**: No pending connections (not an error in non-blocking mode)

#### Lines 6-7: Set Non-Blocking Mode
```cpp
int flags = fcntl(client_fd, F_GETFL, 0);
fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
```
**Purpose**: Make socket operations non-blocking
- **F_GETFL**: Get current file status flags
- **F_SETFL**: Set file status flags
- **O_NONBLOCK**: Non-blocking I/O flag

**Why non-blocking?**
- Prevents `read()/write()` from hanging
- Allows handling multiple clients without threads
- Works with edge-triggered epoll

#### Lines 8-11: Register with epoll
```cpp
struct epoll_event event;
event.events = EPOLLIN | EPOLLET;
event.data.fd = client_fd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
```
**Purpose**: Add client socket to epoll monitoring

**Event flags**:
- **EPOLLIN**: Monitor for input (readable data)
- **EPOLLET**: Edge-triggered mode

**Edge-Triggered vs Level-Triggered**:
```
Level-Triggered (LT):
┌─────────────────────────────────────────────────────────────┐
│ Data Available:  ████████████████████████████████████████   │
│ Notifications:   ↑    ↑    ↑    ↑    ↑    ↑    ↑    ↑     │
│                  (continuous notifications while data exists)│
└─────────────────────────────────────────────────────────────┘

Edge-Triggered (ET):
┌─────────────────────────────────────────────────────────────┐
│ Data Available:  ████████████████████████████████████████   │
│ Notifications:   ↑                                         │
│                  (only when state changes)                 │
└─────────────────────────────────────────────────────────────┘
```

**Why Edge-Triggered?**
- More efficient (fewer system calls)
- Prevents repeated notifications
- Requires non-blocking I/O and complete data reading

#### Lines 12-13: Client Management
```cpp
clients[client_fd] = new Client(client_fd);
clientToServergMap[client_fd] = serverConfigMap[server_fd][0];
```
**Purpose**: Create and store client state
- Create new Client object to manage this connection
- Map client to server configuration (for virtual hosts)

## 2. Edge-Triggered Mode Deep Dive

### What is Edge-Triggered Mode?

Edge-triggered mode means epoll only notifies you when the **state changes**, not when the condition remains true.

### Example Scenario:
```
Time 0: Client sends 1000 bytes
┌─────────────────────────────────────────────────────────────┐
│ Socket Buffer: [1000 bytes of data]                        │
│ epoll event:   EPOLLIN triggered ↑                         │
└─────────────────────────────────────────────────────────────┘

Time 1: Application reads 500 bytes
┌─────────────────────────────────────────────────────────────┐
│ Socket Buffer: [500 bytes remaining]                       │
│ epoll event:   NO notification (state didn't change)       │
└─────────────────────────────────────────────────────────────┘

Time 2: Client sends 200 more bytes
┌─────────────────────────────────────────────────────────────┐
│ Socket Buffer: [700 bytes total]                           │
│ epoll event:   EPOLLIN triggered ↑ (new data arrived)      │
└─────────────────────────────────────────────────────────────┘
```

### Implications for Programming:

1. **Must read all available data** in one go
2. **Must handle EAGAIN/EWOULDBLOCK** properly
3. **Cannot rely on repeated notifications**

### Example of Proper Edge-Triggered Reading:
```cpp
void Server::handleClientRead(int client_fd)
{
    char buffer[4096];
    ssize_t bytes_read;

    // Loop to read ALL available data
    while (true)
    {
        bytes_read = read(client_fd, buffer, sizeof(buffer));
        
        if (bytes_read > 0)
        {
            // Process data
            clients[client_fd]->appendToBuffer(std::string(buffer, bytes_read));
        }
        else if (bytes_read == 0)
        {
            // Client disconnected
            closeClientConnection(client_fd);
            return;
        }
        else
        {
            // bytes_read == -1
            // In non-blocking mode, this usually means EAGAIN
            // (no more data available right now)
            break;
        }
    }
}
```

## 3. sockaddr_in Structure Explained

```cpp
struct sockaddr_in {
    sa_family_t    sin_family;  // Address family (AF_INET for IPv4)
    in_port_t      sin_port;    // Port number (network byte order)
    struct in_addr sin_addr;    // IPv4 address
    char           sin_zero[8]; // Padding to match sockaddr size
};

struct in_addr {
    uint32_t s_addr;            // IPv4 address (network byte order)
};
```

### Visual Representation:
```
sockaddr_in structure (16 bytes total):
┌─────────────┬─────────────┬─────────────┬─────────────┐
│  sin_family │  sin_port   │          sin_addr         │
│   (2 bytes) │  (2 bytes)  │         (4 bytes)         │
├─────────────┴─────────────┴─────────────┬─────────────┤
│            sin_zero (padding)            │
│                 (8 bytes)                │
└──────────────────────────────────────────┘

Example values after accept():
┌─────────────┬─────────────┬─────────────┬─────────────┐
│   AF_INET   │    8080     │   192.168.1.100           │
│      2      │   0x1F90    │    0x6401A8C0             │
├─────────────┴─────────────┴─────────────┬─────────────┤
│              0x00000000                  │
└──────────────────────────────────────────┘
```

### Network Byte Order:
- **Problem**: Different CPU architectures store bytes differently
- **Solution**: Network protocols use "big-endian" byte order
- **Functions**: `htons()`, `ntohs()`, `htonl()`, `ntohl()`

```cpp
// Convert host to network byte order
uint16_t port_host = 8080;              // Host byte order
uint16_t port_network = htons(8080);    // Network byte order

// Convert network to host byte order  
uint16_t port_back = ntohs(port_network); // Back to host order
```

## 4. fcntl() and File Descriptor Flags

### Purpose of fcntl()
File control function for manipulating file descriptor properties.

### Common Operations:
```cpp
// Get current flags
int flags = fcntl(fd, F_GETFL, 0);

// Set non-blocking
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// Set close-on-exec
fcntl(fd, F_SETFD, FD_CLOEXEC);
```

### File Status Flags:
- **O_NONBLOCK**: Non-blocking I/O
- **O_APPEND**: Always append to end of file
- **O_SYNC**: Synchronous I/O

### Why Non-blocking is Critical:
```
Blocking Mode:
┌─────────────────┐
│ read(fd, buf)   │ ──► Waits until data available
│     ⏳          │     (blocks entire thread)
└─────────────────┘

Non-blocking Mode:
┌─────────────────┐
│ read(fd, buf)   │ ──► Returns immediately
│   returns -1    │     with errno = EAGAIN
│   errno=EAGAIN  │     if no data available
└─────────────────┘
```

## 5. Memory Management in Client Handling

### Client Object Lifecycle:
```cpp
// Creation (in acceptNewConnection)
clients[client_fd] = new Client(client_fd);

// Usage (in handleClientRead/Write)
Client* client = clients[client_fd];
client->appendToBuffer(data);

// Cleanup (in closeClientConnection)
std::map<int, Client*>::iterator it = clients.find(client_fd);
if (it != clients.end()) {
    delete it->second;    // Free memory
    clients.erase(it);    // Remove from map
}
```

### Memory Layout:
```
Heap Memory:
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Client Object │    │   Client Object │    │   Client Object │
│      fd: 6      │    │      fd: 7      │    │      fd: 8      │
│   buffer: "GET  │    │   buffer: "POST │    │   buffer: ""    │
│   ...           │    │   ...           │    │   ...           │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         ▲                       ▲                       ▲
         │                       │                       │
Stack (clients map):
┌─────────────────────────────────────────────────────────────┐
│  clients[6] ──────────────────┘                             │
│  clients[7] ────────────────────────────────┘               │  
│  clients[8] ──────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Potential Memory Issues:
1. **Memory leaks**: Forgetting to delete Client objects
2. **Use-after-free**: Accessing deleted Client objects
3. **Double-free**: Deleting same object twice

### Protection Mechanisms:
```cpp
// Safe deletion pattern
std::map<int, Client*>::iterator it = clients.find(client_fd);
if (it != clients.end()) {
    delete it->second;
    it->second = NULL;  // Optional: prevent use-after-free
    clients.erase(it);
}
```
