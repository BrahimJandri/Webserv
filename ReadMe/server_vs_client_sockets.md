# Server Socket vs Client Socket: Detailed Comparison

## 1. Server Socket (Listening Socket)

### Purpose
- **Listens** for incoming connections on a specific port
- **Accepts** new client connections
- **Does NOT** exchange data directly with clients

### Characteristics
```cpp
// Server socket creation
int server_fd = socket(AF_INET, SOCK_STREAM, 0);  // Create socket
bind(server_fd, &addr, sizeof(addr));             // Bind to address:port
listen(server_fd, 10);                            // Start listening (backlog=10)

// Add to epoll for monitoring
struct epoll_event ev;
ev.events = EPOLLIN;        // Monitor for incoming connections
ev.data.fd = server_fd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);
```

### Data Structures
```cpp
std::set<int> server_fds;  // Contains all server socket file descriptors
std::map<int, std::vector<ConfigParser::ServerConfig>> serverConfigMap;
// Maps server_fd → list of server configurations (virtual hosts)
```

### State Diagram
```
Server Socket Lifecycle:
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Created   │ → │    Bound    │ → │  Listening  │ → │   Closed    │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
     socket()          bind()           listen()           close()
```

### When EPOLLIN Event Occurs
```
Server Socket EPOLLIN Event:
┌─────────────────────────────────────┐
│ Client attempts connection          │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ Kernel queues connection request    │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ epoll_wait() returns server_fd      │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ acceptNewConnection(server_fd)      │
│ calls accept() to get client_fd     │
└─────────────────────────────────────┘
```

## 2. Client Socket (Connected Socket)

### Purpose
- **Exchanges** data with a specific client
- **Reads** HTTP requests from client
- **Writes** HTTP responses to client

### Characteristics
```cpp
// Client socket creation (via accept)
int client_fd = accept(server_fd, &client_addr, &client_len);  // Accept connection
fcntl(client_fd, F_SETFL, O_NONBLOCK);                       // Set non-blocking

// Add to epoll for monitoring
struct epoll_event event;
event.events = EPOLLIN | EPOLLET;  // Monitor for data + edge-triggered
event.data.fd = client_fd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
```

### Data Structures
```cpp
std::map<int, Client*> clients;  // Maps client_fd → Client object
std::map<int, ConfigParser::ServerConfig> clientToServerMap;
// Maps client_fd → server configuration for this client
```

### State Machine
```
Client Socket State Machine:

┌─────────────┐    EPOLLIN     ┌─────────────┐    Request      ┌─────────────┐
│   Reading   │ ◄─────────────│   Waiting   │    Complete    │ Processing  │
│   Request   │               │ for Data    │ ──────────────▶│  Request    │
└─────────────┘               └─────────────┘                └─────────────┘
      │                              ▲                              │
      │ Partial Read                 │                              │
      └──────────────────────────────┘                              │ Response Ready
                                                                     │
┌─────────────┐    EPOLLOUT    ┌─────────────┐ ◄───────────────────┘
│   Sending   │ ◄─────────────│  Ready to   │
│  Response   │               │    Send     │
└─────────────┘               └─────────────┘
      │
      │ Response Sent
      ▼
┌─────────────┐
│   Closed    │
└─────────────┘
```

### When EPOLLIN Event Occurs
```
Client Socket EPOLLIN Event:
┌─────────────────────────────────────┐
│ Client sends HTTP request data      │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ Data available in socket buffer     │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ epoll_wait() returns client_fd      │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ handleClientRead(client_fd)         │
│ reads data into Client buffer       │
└─────────────────────────────────────┘
```

### When EPOLLOUT Event Occurs
```
Client Socket EPOLLOUT Event:
┌─────────────────────────────────────┐
│ Socket buffer has space for writing │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ epoll_wait() returns client_fd      │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ handleClientWrite(client_fd)        │
│ sends response data to client       │
└─────────────────────────────────────┘
```

## 3. Key Differences Summary

| Aspect | Server Socket | Client Socket |
|--------|---------------|---------------|
| **Purpose** | Listen for connections | Exchange data with client |
| **Creation** | socket() + bind() + listen() | accept() from server socket |
| **Events** | EPOLLIN only (new connections) | EPOLLIN (read) + EPOLLOUT (write) |
| **Data Flow** | No data exchange | HTTP request/response |
| **Lifetime** | Long-lived (entire server lifetime) | Short-lived (per client session) |
| **Count** | Few (one per listening port) | Many (one per active client) |
| **Blocking** | Can be blocking or non-blocking | Non-blocking (set with fcntl) |

## 4. Visual Representation of the Complete Flow

```
Time: T1 - Server Start
┌─────────────────┐
│  Server Socket  │  EPOLLIN monitoring
│   Port 8080     │  (waiting for connections)
│     fd: 3       │
└─────────────────┘

Time: T2 - Client Connects
┌─────────────────┐                    ┌─────────────────┐
│  Server Socket  │ ──── accept() ───▶ │  Client Socket  │
│   Port 8080     │                    │    Client A     │
│     fd: 3       │                    │     fd: 6       │
└─────────────────┘                    └─────────────────┘
      │                                         │
      │ Still monitoring                        │ EPOLLIN monitoring
      │ for new connections                     │ (waiting for HTTP request)

Time: T3 - Client Sends Request
┌─────────────────┐                    ┌─────────────────┐
│  Server Socket  │                    │  Client Socket  │
│   Port 8080     │                    │    Client A     │
│     fd: 3       │                    │     fd: 6       │
└─────────────────┘                    └─────────────────┘
      │                                         │
      │ EPOLLIN monitoring                      │ EPOLLIN event triggered
      │                                         │ handleClientRead()

Time: T4 - Server Processes and Responds
┌─────────────────┐                    ┌─────────────────┐
│  Server Socket  │                    │  Client Socket  │
│   Port 8080     │                    │    Client A     │
│     fd: 3       │                    │     fd: 6       │
└─────────────────┘                    └─────────────────┘
      │                                         │
      │ EPOLLIN monitoring                      │ EPOLLOUT monitoring
      │                                         │ (ready to send response)

Time: T5 - Response Sent, Connection Closed
┌─────────────────┐                    ┌─────────────────┐
│  Server Socket  │                    │  Client Socket  │
│   Port 8080     │                    │    Client A     │
│     fd: 3       │                    │   [CLOSED]      │
└─────────────────┘                    └─────────────────┘
      │
      │ EPOLLIN monitoring
      │ (ready for next client)
```

## 5. Memory Management Differences

### Server Socket
- **Allocated once** during server startup
- **Stored in**: `std::set<int> server_fds`
- **Cleaned up**: Only during server shutdown

### Client Socket
- **Allocated per connection** in `acceptNewConnection()`
- **Stored in**: `std::map<int, Client*> clients`
- **Cleaned up**: After each client disconnects in `closeClientConnection()`

```cpp
// Client creation (in acceptNewConnection)
clients[client_fd] = new Client(client_fd);  // Allocate new Client object

// Client cleanup (in closeClientConnection)
std::map<int, Client*>::iterator it = clients.find(client_fd);
if (it != clients.end()) {
    delete it->second;     // Free Client object
    clients.erase(it);     // Remove from map
}
close(client_fd);          // Close socket
```
