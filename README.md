# Complete Request Handling Flow

## 1. High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                   Server Process                                │
│                                                                                 │
│  ┌─────────────────┐              ┌─────────────────┐                          │
│  │     epoll_fd    │              │   Signal Handler │                          │
│  │  (Event Loop)   │              │   (SIGINT/QUIT) │                          │
│  └─────────────────┘              └─────────────────┘                          │
│           │                                │                                   │
│           │ monitors                       │ sets _turnoff=1                   │
│           ▼                                ▼                                   │
│  ┌─────────────────────────────────────────────────────────────────────────┐  │
│  │                        File Descriptors                                 │  │
│  │                                                                         │  │
│  │  Server Sockets:          Client Sockets:                              │  │
│  │  ┌─────────────┐          ┌─────────────┐  ┌─────────────┐            │  │
│  │  │   fd: 3     │          │   fd: 6     │  │   fd: 7     │            │  │
│  │  │  Port 80    │          │  Client A   │  │  Client B   │   ...      │  │
│  │  │  EPOLLIN    │          │  EPOLLIN/   │  │  EPOLLOUT   │            │  │
│  │  └─────────────┘          │  EPOLLOUT   │  └─────────────┘            │  │
│  │  ┌─────────────┐          └─────────────┘                              │  │
│  │  │   fd: 4     │                                                       │  │
│  │  │ Port 443    │                                                       │  │
│  │  │  EPOLLIN    │                                                       │  │
│  │  └─────────────┘                                                       │  │
│  └─────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## 2. Detailed Event Processing Flow

```
Start: handleConnections()
         │
         ▼
┌─────────────────┐
│ Setup Signals   │ ──► signal(SIGINT, sighandler)
│ SIGINT, SIGQUIT │     signal(SIGQUIT, sighandler)  
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Declare events  │ ──► struct epoll_event events[MAX_EVENTS];
│ array [64]      │
└─────────────────┘
         │
         ▼
┌─────────────────┐     YES    ┌─────────────────┐
│ !_turnoff ?     │ ─────────► │ Continue Loop   │
└─────────────────┘            └─────────────────┘
         │ NO                            │
         ▼                               ▼
┌─────────────────┐            ┌─────────────────┐
│ Exit Function   │            │ epoll_wait()    │ ──► Block until events ready
└─────────────────┘            │ (blocking call) │     Returns number of events
                               └─────────────────┘
                                        │
                                        ▼
                               ┌─────────────────┐
                               │ Error Check     │ ──► if (num_events == -1)
                               └─────────────────┘            │
                                        │                     ▼
                                        │              ┌─────────────────┐
                                        │              │ errno==EINTR?   │
                                        │              │ (signal inter.) │
                                        │              └─────────────────┘
                                        │                     │ YES
                                        │                     ▼
                                        │              ┌─────────────────┐
                                        │              │ continue loop   │
                                        │              └─────────────────┘
                                        ▼
                               ┌─────────────────┐
                               │ For each event  │ ──► for (i = 0; i < num_events; ++i)
                               │   (i = 0 to n)  │
                               └─────────────────┘
                                        │
                                        ▼
                               ┌─────────────────┐
                               │ Get fd from     │ ──► int fd = events[i].data.fd;
                               │ event[i]        │
                               └─────────────────┘
                                        │
                                        ▼
                               ┌─────────────────┐
                               │ Check fd type   │
                               └─────────────────┘
                                        │
                    ┌───────────────────┼───────────────────┐
                    ▼                   ▼                   ▼
           ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
           │ Server Socket?  │ │ Client EPOLLIN? │ │Client EPOLLOUT? │
           │ (new connection)│ │ (read data)     │ │ (write data)    │
           └─────────────────┘ └─────────────────┘ └─────────────────┘
                    │                   │                   │
                    ▼                   ▼                   ▼
           ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
           │acceptNewConnect │ │handleClientRead │ │handleClientWrite│
           └─────────────────┘ └─────────────────┘ └─────────────────┘
```

## 3. Client Connection Lifecycle

### Phase 1: Connection Establishment
```
Client                   Network                 Server
  │                        │                       │
  │──── TCP SYN ──────────►│                       │
  │                        │──── TCP SYN ────────►│ 
  │                        │                       │ accept() called
  │                        │◄─── TCP SYN+ACK ─────│ new client_fd created
  │◄─── TCP SYN+ACK ──────│                       │
  │                        │                       │
  │──── TCP ACK ──────────►│                       │
  │                        │──── TCP ACK ────────►│ Connection established
  │                        │                       │
                                                   │
                                          ┌─────────────────┐
                                          │ Add to epoll    │
                                          │ EPOLLIN | EPOLLET│
                                          │ Create Client   │
                                          │ object          │
                                          └─────────────────┘
```

### Phase 2: Request Processing
```
Client                   Network                 Server
  │                        │                       │
  │──── HTTP Request ─────►│                       │
  │ GET /index.html        │──── Data ──────────►│ EPOLLIN event
  │ Host: example.com      │                       │
  │ ...                    │                       │ handleClientRead()
  │                        │                       │ ┌─────────────────┐
  │                        │                       │ │ read() data     │
  │                        │                       │ │ parse request   │
  │                        │                       │ │ prepare response│
  │                        │                       │ │ switch to       │
  │                        │                       │ │ EPOLLOUT        │
  │                        │                       │ └─────────────────┘
```

### Phase 3: Response Sending
```
Client                   Network                 Server
  │                        │                       │ EPOLLOUT event
  │                        │                       │
  │                        │                       │ handleClientWrite()
  │                        │                       │ ┌─────────────────┐
  │                        │◄─── HTTP Response────│ │ write() response│
  │◄─── HTTP Response─────│                       │ │ close connection│
  │ HTTP/1.1 200 OK        │                       │ └─────────────────┘
  │ Content-Type: text/html│                       │
  │ ...                    │                       │
  │                        │                       │
```

## 4. Memory State Transitions

### Server Startup:
```
Memory State at Startup:
┌─────────────────────────────────────────────────────────────┐
│ Server Object:                                              │
│ ┌─────────────────┐                                         │
│ │ epoll_fd: 3     │                                         │
│ │ server_fds: {}  │                                         │  
│ │ clients: {}     │                                         │
│ │ ...             │                                         │
│ └─────────────────┘                                         │
└─────────────────────────────────────────────────────────────┘
```

### After setupServers():
```
Memory State after setupServers():
┌─────────────────────────────────────────────────────────────┐
│ Server Object:                                              │
│ ┌─────────────────┐                                         │
│ │ epoll_fd: 3     │                                         │
│ │ server_fds:     │                                         │
│ │   {4, 5}        │ ──► Server sockets for ports 80, 443   │
│ │ clients: {}     │                                         │
│ │ serverConfigMap:│                                         │
│ │   4 → [Config1] │ ──► Port 80 config                     │
│ │   5 → [Config2] │ ──► Port 443 config                    │
│ └─────────────────┘                                         │
└─────────────────────────────────────────────────────────────┘
```

### After Client Connections:
```
Memory State with Active Clients:
┌─────────────────────────────────────────────────────────────┐
│ Server Object:                                              │
│ ┌─────────────────┐         ┌─────────────────┐            │
│ │ epoll_fd: 3     │         │ Client Objects: │            │
│ │ server_fds:     │         │                 │            │
│ │   {4, 5}        │         │ ┌─────────────┐ │ fd: 6      │
│ │ clients:        │ ──────► │ │   Buffer    │ │ Client A   │
│ │   6 → Client*   │         │ │   Request   │ │            │
│ │   7 → Client*   │         │ │   Response  │ │            │
│ │   8 → Client*   │         │ └─────────────┘ │            │
│ │ ...             │         │ ┌─────────────┐ │ fd: 7      │
│ └─────────────────┘         │ │   Buffer    │ │ Client B   │
│                             │ │   Request   │ │            │
│                             │ │   Response  │ │            │
│                             │ └─────────────┘ │            │
│                             │ ...             │            │
│                             └─────────────────┘            │
└─────────────────────────────────────────────────────────────┘

epoll Interest List:
┌─────────────────────────────────────────────────────────────┐
│ fd: 4 (server) → EPOLLIN     (new connections)             │
│ fd: 5 (server) → EPOLLIN     (new connections)             │
│ fd: 6 (client) → EPOLLIN     (read request)                │
│ fd: 7 (client) → EPOLLOUT    (write response)              │
│ fd: 8 (client) → EPOLLIN     (read request)                │
└─────────────────────────────────────────────────────────────┘
```

## 5. Error Scenarios and Handling

### Scenario 1: Client Disconnects Abruptly
```
Normal Flow:            Error Flow:
     │                       │
     ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│ EPOLLIN event   │    │ EPOLLIN event   │
└─────────────────┘    └─────────────────┘
     │                       │
     ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│ read() returns  │    │ read() returns  │
│ > 0 bytes       │    │ 0 bytes         │
└─────────────────┘    └─────────────────┘
     │                       │
     ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│ Process data    │    │ Client          │
│ Continue        │    │ disconnected    │
└─────────────────┘    └─────────────────┘
                            │
                            ▼
                    ┌─────────────────┐
                    │ closeClient     │
                    │ Connection()    │
                    └─────────────────┘
```

### Scenario 2: epoll_wait() Interrupted by Signal
```
Signal Flow:
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ User presses    │    │ Kernel delivers │    │ sighandler()    │
│ Ctrl+C          │ ──►│ SIGINT signal   │ ──►│ sets _turnoff=1 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                       │
                                                       ▼
                                              ┌─────────────────┐
                                              │ epoll_wait()    │
                                              │ returns -1      │
                                              │ errno = EINTR   │
                                              └─────────────────┘
                                                       │
                                                       ▼
                                              ┌─────────────────┐
                                              │ continue loop   │
                                              │ check _turnoff  │
                                              └─────────────────┘
                                                       │
                                                       ▼
                                              ┌─────────────────┐
                                              │ _turnoff == 1   │
                                              │ Exit loop       │
                                              │ Cleanup         │
                                              └─────────────────┘
```

## 6. Performance Characteristics

### Scalability Comparison:
```
Traditional Threading Model:
Memory per client: ~8MB (thread stack)
1000 clients = ~8GB RAM
Context switching overhead

epoll-based Model:
Memory per client: ~1KB (Client object)
1000 clients = ~1MB RAM
No context switching (single thread)

Performance Graph:
Connections
     ▲
     │      ┌─── epoll model
  10K│     ╱
     │    ╱
   1K│   ╱
     │  ╱ ┌─── threading model
  100│ ╱ ╱
     │╱ ╱
   10├╱─────────────────► CPU/Memory Usage
     0  10   100   1K   10K
```

### Time Complexity:
- **epoll_wait()**: O(number of ready fds)
- **Traditional select()**: O(total number of fds)
- **Client lookup**: O(log n) using std::map
- **Server lookup**: O(log n) using std::set

This multiplexing implementation provides efficient, scalable handling of thousands of concurrent connections using a single thread and the power of Linux's epoll mechanism.
