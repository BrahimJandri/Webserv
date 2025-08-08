# Webserv Complete Flow Map

## 1. System Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                 WEBSERV ARCHITECTURE                                │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐           │
│  │   Config    │    │   Server    │    │   Client    │    │    HTTP     │           │
│  │   Parser    │──▶│   Manager   │───▶│   Handler   │──▶│  Processor  │           │
│  └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘           │
│         │                   │                   │                   │               │
│         ▼                   ▼                   ▼                   ▼               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐           │
│  │ server.conf │    │ epoll_fd +  │    │ client_fd + │    │ GET/POST/   │           │
│  │ locations   │    │ server_fds  │    │ Client obj  │    │ DELETE      │           │
│  │ error pages │    │ listening   │    │ buffers     │    │ responses   │           │
│  └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘           │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## 2. Complete Data Flow: Client Request Journey

```
Step 1: SERVER STARTUP
┌────────────────────────────────────────────────────────────────────────────────────┐
│ main() → Server::run()                                                             │
│                                                                                    │
│ 1. Parse config file   ──┐                                                         │
│ 2. Create server sockets │  ┌─────────── Config Parser  ──────────────┐            │
│ 3. bind() + listen()     │  │ server.conf:                            │            │
│ 4. Add to epoll          │  │ server {                                │            │
│                          │  │   listen 8080;                          │            │
│                          └▶│   server_name example.com;              │            │
│                             │   location / { root /var/www; }         │            │
│                             │ }                                       │            │
│                             └─────────────────────────────────────────┘            │
│                                                                                    │
│ Result: server_fd listening on port 8080, added to epoll                           │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 2: WAITING FOR CONNECTIONS
┌────────────────────────────────────────────────────────────────────────────────────┐
│ epoll_wait() monitoring server_fd for EPOLLIN events                               │
│                                                                                    │
│ ┌─────────────────┐                                                                │
│ │  Server Socket  │  ◄── epoll_fd monitoring                                       │
│ │   Port 8080     │                                                                │
│ │     fd: 3       │  Events: EPOLLIN (waiting for new connections)                 │
│ │   [LISTENING]   │                                                                │
│ └─────────────────┘                                                                │
│                                                                                    │
│ Data Structures:                                                                   │
│ • server_fds = {3}                                                                 │
│ • serverConfigMap[3] = [ServerConfig for port 8080]                                │
│ • clients = {} (empty)                                                             │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 3: CLIENT CONNECTION ARRIVES
┌────────────────────────────────────────────────────────────────────────────────────┐
│ Client connects → kernel queues connection → epoll_wait() returns server_fd        │
│                                                                                    │
│        Internet                    Server                                          │
│     ┌─────────────┐               ┌─────────────┐                                  │
│     │   Client    │ ──TCP SYN──▶ │   Server    │                                   │
│     │ 192.168.1.5 │ ◄─TCP SYN+ACK │   Socket    │                                  │
│     │             │ ──TCP ACK──▶ │   fd: 3     │                                   │
│     └─────────────┘               └─────────────┘                                  │
│                                         │                                          │
│                                         ▼                                          │
│                                  acceptNewConnection(3)                            │
│                                                                                    │
│ Kernel's connection queue:                                                         │
│ ┌───────────────────────────────┐                                                  │
│ │ [Client 192.168.1.5:45678]  ──┼── Ready for accept()                             │
│ └───────────────────────────────┘                                                  │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 4: ACCEPT NEW CONNECTION
┌────────────────────────────────────────────────────────────────────────────────────┐
│ acceptNewConnection(server_fd) execution:                                          │
│                                                                                    │
│ int client_fd = accept(server_fd, &client_addr, &client_len);                      │
│ ├─ Creates new socket fd: 6                                                        │
│ ├─ Extracts client info: 192.168.1.5:45678                                         │
│ └─ Connection established                                                          │
│                                                                                    │
│ fcntl(client_fd, F_SETFL, O_NONBLOCK);                                             │
│ ├─ Makes socket non-blocking                                                       │
│ └─ Prevents read/write from hanging                                                │
│                                                                                    │
│ epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, EPOLLIN | EPOLLET);                  │
│ ├─ Adds client_fd to epoll monitoring                                              │
│ ├─ EPOLLIN: monitor for incoming data                                              │
│ └─ EPOLLET: edge-triggered mode                                                    │
│                                                                                    │
│ clients[client_fd] = new Client(client_fd);                                        │
│ clientToServerMap[client_fd] = serverConfigMap[server_fd][0];                      │
│                                                                                    │
│ Result State:                                                                      │
│ ┌─────────────────┐    ┌─────────────────┐                                         │
│ │  Server Socket  │    │  Client Socket  │                                         │
│ │   Port 8080     │    │    Client A     │                                         │
│ │     fd: 3       │    │     fd: 6       │                                         │
│ │   [LISTENING]   │    │  [CONNECTED]    │                                         │
│ └─────────────────┘    └─────────────────┘                                         │
│         │                       │                                                  │
│         │ EPOLLIN monitoring    │ EPOLLIN monitoring                               │
│         │ (new connections)     │ (HTTP request data)                              │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 5: CLIENT SENDS HTTP REQUEST
┌────────────────────────────────────────────────────────────────────────────────────┐
│ Client sends HTTP request → socket buffer receives data → EPOLLIN event            │
│                                                                                    │
│     Client Side                           Server Side                              │
│ ┌─────────────────┐                   ┌─────────────────┐                          │
│ │ HTTP Request    │ ───TCP packets──▶ │ Socket Buffer   │                          │
│ │ GET / HTTP/1.1  │                   │ [HTTP data...]  │                          │
│ │ Host: localhost │                   │     fd: 6       │                          │
│ │ ...             │                   └─────────────────┘                          │
│ └─────────────────┘                           │                                    │
│                                               ▼                                    │
│                                      epoll_wait() returns                          │
│                                      events[i].data.fd = 6                         │
│                                      events[i].events = EPOLLIN                    │
│                                                                                    │
│ Network Packet Flow:                                                               │
│ ┌─────────┬─────────┬─────────┬─────────┬─────────┐                                │
│ │ Eth Hdr │ IP Hdr  │ TCP Hdr │  HTTP   │ Payload │                                │
│ │ 14bytes │ 20bytes │ 20bytes │ Headers │  Data   │                                │
│ └─────────┴─────────┴─────────┴─────────┴─────────┘                                │
│                                   │                                                │
│                                   └─ Kernel extracts and places in socket buffer   │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 6: HANDLE CLIENT READ (Edge-Triggered)
┌────────────────────────────────────────────────────────────────────────────────────┐
│ handleClientRead(client_fd) execution:                                             │
│                                                                                    │
│ char buffer[4096];                                                                 │
│ while (true) {                                                                     │
│     ssize_t bytes_read = read(client_fd, buffer, 4096);                            │
│     if (bytes_read > 0) {                                                          │
│         clients[client_fd]->appendToBuffer(buffer, bytes_read);                    │
│     } else if (bytes_read == 0) {                                                  │
│         // Client disconnected                                                     │
│         closeClientConnection(client_fd);                                          │
│     } else if (errno == EAGAIN) {                                                  │
│         // No more data available (edge-triggered)                                 │
│         break;                                                                     │
│     }                                                                              │
│ }                                                                                  │
│                                                                                    │
│ Edge-Triggered Behavior:                                                           │
│ ┌─────────────────────────────────────────────────────────────────────────────┐    │
│ │ Data arrives: ████████████████████████                                      │    │
│ │ Notification:     ↑                                                         │    │
│ │ Must read ALL:    └── read() loop until EAGAIN                              │    │
│ └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                    │
│ Client Buffer State:                                                               │
│ ┌─────────────────────────────────────────────────────────────────────────────┐    │
│ │ "GET / HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl\r\n\r\n"        │    │
│ └─────────────────────────────────────────────────────────────────────────────┘    │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 7: HTTP REQUEST PARSING
┌────────────────────────────────────────────────────────────────────────────────────┐
│ Client::parseRequest() - State Machine Execution:                                  │
│                                                                                    │
│ Raw Buffer: "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"                       │
│                                                                                    │
│ Parsing State Machine:                                                             │
│ ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐           │
│ │   READING   │──▶│   PARSING   │──▶ │ PROCESSING  │──▶│   SENDING   │           │
│ │   REQUEST   │    │   HEADERS   │    │   REQUEST   │    │  RESPONSE   │           │
│ └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘           │
│                                                                                    │
│ Extracted Components:                                                              │
│ ┌─────────────────────────────────────────────────────────────────────────────┐    │
│ │ Method: "GET"                                                               │    │
│ │ Path: "/"                                                                   │    │
│ │ Version: "HTTP/1.1"                                                         │    │
│ │ Headers: {                                                                  │    │
│ │   "Host": "localhost:8080",                                                 │    │
│ │   "User-Agent": "curl/7.68.0"                                               │    │
│ │ }                                                                           │    │
│ │ Body: "" (empty for GET)                                                    │    │
│ └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                    │
│ Request Validation:                                                                │
│ ✓ Valid HTTP method                                                                │
│ ✓ Valid path format                                                                │
│ ✓ Host header present                                                              │
│ ✓ Complete request received                                                        │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 8: REQUEST PROCESSING & ROUTING
┌────────────────────────────────────────────────────────────────────────────────────┐
│ Server::processRequest(client_fd) execution:                                       │
│                                                                                    │
│ 1. Virtual Host Resolution:                                                        │
│    Host: "localhost:8080" → ServerConfig for port 8080                             │
│                                                                                    │
│ 2. Location Matching:                                                              │
│    Request path "/" matches location "/" in config                                 │
│    ┌─────────────────────────────────────────────────────────────────────────┐     │
│    │ location / {                                                            │     │
│    │   root /var/www/html;                                                   │     │
│    │   index index.html index.htm;                                           │     │
│    │   allowed_methods GET POST;                                             │     │
│    │ }                                                                       │     │
│    └─────────────────────────────────────────────────────────────────────────┘     │
│                                                                                    │
│ 3. Method Validation:                                                              │
│    GET ∈ {GET, POST} ✓ Allowed                                                    │
│                                                                                    │
│ 4. File System Resolution:                                                         │
│    root="/var/www/html" + path="/" → "/var/www/html/"                              │
│    Directory found → check for index files                                         │
│    "/var/www/html/index.html" exists ✓                                             │
│                                                                                    │
│ 5. File Access & Reading:                                                          │
│    open("/var/www/html/index.html", O_RDONLY)                                      │
│    read() file content into response buffer                                        │
│                                                                                    │
│ File System Layout:                                                                │
│ /var/www/html/                                                                     │
│ ├── index.html      ← Target file                                                  │
│ ├── about.html                                                                     │
│ ├── css/                                                                           │
│ │   └── style.css                                                                  │
│ └── images/                                                                        │
│     └── logo.png                                                                   │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 9: HTTP RESPONSE GENERATION
┌────────────────────────────────────────────────────────────────────────────────────┐
│ generateResponse() execution:                                                      │
│                                                                                    │
│ Response Components Assembly:                                                      │
│                                                                                    │
│ 1. Status Line:                                                                    │
│    "HTTP/1.1 200 OK\r\n"                                                           │
│                                                                                    │
│ 2. Headers Generation:                                                             │
│    Content-Type: text/html (based on .html extension)                              │
│    Content-Length: 1234 (actual file size)                                         │
│    Date: Thu, 01 Aug 2025 10:30:00 GMT                                             │
│    Server: Webserv/1.0                                                             │
│    Connection: close                                                               │
│                                                                                    │
│ 3. Complete Response Buffer:                                                       │
│ ┌─────────────────────────────────────────────────────────────────────────────┐    │
│ │ "HTTP/1.1 200 OK\r\n"                                                       │    │
│ │ "Content-Type: text/html\r\n"                                               │    │
│ │ "Content-Length: 1234\r\n"                                                  │    │
│ │ "Date: Thu, 01 Aug 2025 10:30:00 GMT\r\n"                                   │    │
│ │ "Server: Webserv/1.0\r\n"                                                   │    │
│ │ "Connection: close\r\n"                                                     │    │
│ │ "\r\n"                                                                      │    │
│ │ "<!DOCTYPE html>                                                            │    │
│ │ <html><head><title>Welcome</title></head>                                   │    │
│ │ <body><h1>Hello World!</h1></body></html>"                                  │    │
│ └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                    │
│ Response stored in: clients[client_fd]->response_buffer                            │
│ State changed to: SENDING_RESPONSE                                                 │
│ epoll events modified to: EPOLLOUT (ready to write)                                │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 10: SENDING RESPONSE (EPOLLOUT Event)
┌────────────────────────────────────────────────────────────────────────────────────┐
│ epoll_wait() returns client_fd with EPOLLOUT event                                 │
│ handleClientWrite(client_fd) execution:                                            │
│                                                                                    │
│ ssize_t bytes_sent = write(client_fd, response_buffer, response_size);             │
│                                                                                    │
│ Write Scenarios:                                                                   │
│ ┌─────────────────────────────────────────────────────────────────────────────┐    │
│ │ Case 1: All data sent                                                       │    │
│ │ bytes_sent == response_size → Response complete                             │    │
│ │                                                                             │    │
│ │ Case 2: Partial write                                                       │    │
│ │ bytes_sent < response_size → Update buffer, continue later                  │    │
│ │                                                                             │    │
│ │ Case 3: Would block                                                         │    │
│ │ bytes_sent == -1 && errno == EAGAIN → Try again later                       │    │
│ └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                    │
│ Network Transmission:                                                              │
│     Server Side                           Client Side                              │
│ ┌─────────────────┐                   ┌─────────────────┐                          │
│ │ Response Buffer │ ───TCP packets─▶ │ Client receives │                          │
│ │ HTTP/1.1 200 OK │                   │ HTTP response   │                          │
│ │ Content-Type... │                   │ Displays page   │                          │
│ │ <html>...       │                   └─────────────────┘                          │
│ └─────────────────┘                                                                │
│                                                                                    │
│ TCP Packet Segmentation:                                                           │
│ Large responses split into multiple TCP segments                                   │
│ ┌─────────┬─────────┬─────────┬─────────┐                                          │
│ │Segment 1│Segment 2│Segment 3│Segment 4│                                          │
│ │Headers  │HTML pt1 │HTML pt2 │HTML pt3 │                                          │
│ └─────────┴─────────┴─────────┴─────────┘                                          │
└────────────────────────────────────────────────────────────────────────────────────┘

Step 11: CONNECTION CLEANUP
┌────────────────────────────────────────────────────────────────────────────────────┐
│ closeClientConnection(client_fd) execution:                                        │
│                                                                                    │
│ 1. Remove from epoll monitoring:                                                   │
│    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);                            │
│                                                                                    │
│ 2. Close socket:                                                                   │
│    close(client_fd);  // Sends FIN packet to client                                │
│                                                                                    │
│ 3. Memory cleanup:                                                                 │
│    delete clients[client_fd];     // Free Client object                            │
│    clients.erase(client_fd);       // Remove from map                              │
│    clientToServerMap.erase(client_fd);  // Remove config mapping                   │
│                                                                                    │
│ 4. TCP Connection Termination:                                                     │
│    ┌─────────────┐              ┌─────────────┐                                    │
│    │   Server    │ ───FIN────▶  │   Client    │                                   │
│    │             │ ◄──FIN+ACK── │             │                                    │
│    │             │ ───ACK────▶  │             │                                   │
│    └─────────────┘              └─────────────┘                                    │
│                                                                                    │
│ Memory State After Cleanup:                                                        │
│ ┌─────────────────────────────────────────────────────────────────────────────┐    │
│ │ clients = {} (empty)                                                        │    │
│ │ clientToServerMap = {} (empty)                                              │    │
│ │ server_fds = {3} (still listening)                                          │    │
│ │ Heap memory: Client object freed                                            │    │
│ └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                    │
│ Server returns to Step 2: Waiting for next connection                              │
└────────────────────────────────────────────────────────────────────────────────────┘
```

## 3. Error Handling & Edge Cases

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                ERROR SCENARIOS                                      │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│ A. Client Disconnects During Request:                                               │
│    read() returns 0 → closeClientConnection()                                       │
│                                                                                     │
│ B. Malformed HTTP Request:                                                          │
│    Parse error → Generate 400 Bad Request response                                  │
│                                                                                     │
│ C. File Not Found:                                                                  │
│    stat() fails → Generate 404 Not Found response                                   │
│                                                                                     │
│ D. Method Not Allowed:                                                              │
│    POST to GET-only location → Generate 405 Method Not Allowed                      │
│                                                                                     │
│ E. Socket Buffer Full (EPOLLOUT):                                                   │
│    write() returns EAGAIN → Wait for next EPOLLOUT event                            │
│                                                                                     │
│ F. Server Socket Error:                                                             │
│    accept() fails → Log error, continue listening                                   │
│                                                                                     │
│ G. Memory Allocation Failure:                                                       │
│    new Client() fails → Close connection, clean up                                  │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## 4. Key Data Structures State Tracking

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              MEMORY STATE DIAGRAM                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│ Server Process Memory Layout:                                                       │
│                                                                                     │
│ ┌─────────────────────────────────────────────────────────────────────────────┐     │
│ │                              STACK                                          │     │
│ │ ┌─────────────────┐                                                         │     │
│ │ │ main() frame    │                                                         │     │
│ │ │ Server server   │                                                         │     │
│ │ │ epoll_fd = 4    │                                                         │     │
│ │ └─────────────────┘                                                         │     │
│ │ ┌─────────────────┐                                                         │     │
│ │ │ run() frame     │                                                         │     │
│ │ │ events[1024]    │                                                         │     │
│ │ │ nfds            │                                                         │     │
│ │ └─────────────────┘                                                         │     │
│ └─────────────────────────────────────────────────────────────────────────────┘     │
│                                                                                     │
│ ┌─────────────────────────────────────────────────────────────────────────────┐     │
│ │                              HEAP                                           │     │
│ │                                                                             │     │
│ │ std::set<int> server_fds:                                                   │     │
│ │ ┌─────┬─────┬─────┐                                                         │     │
│ │ │  3  │  7  │ 11  │  (listening on ports 8080, 8081, 8082)                  │     │
│ │ └─────┴─────┴─────┘                                                         │     │
│ │                                                                             │     │
│ │ std::map<int, Client*> clients:                                             │     │
│ │ ┌────────┬──────────────┐ ┌────────┬──────────────┐                         │     │
│ │ │   6    │ Client* ──────┼─│   8    │ Client* ──────┼─┐                     │     │
│ │ └────────┴──────────────┘ └────────┴──────────────┘ │                       │     │
│ │                                                      │                      │     │
│ │ Client Objects:                                      │                      │     │
│ │ ┌─────────────────────────────────────────────────┐  │                      │     │
│ │ │ Client(fd=6):                                   │  │                      │     │
│ │ │ • state = READING_REQUEST                       │◄─┘                      │     │
│ │ │ • request_buffer = "GET / HTTP/1.1..."         │                          │     │
│ │ │ • response_buffer = ""                          │                         │     │
│ │ │ • bytes_sent = 0                                │                         │     │
│ │ └─────────────────────────────────────────────────┘                         │     │
│ │ ┌─────────────────────────────────────────────────┐                         │     │
│ │ │ Client(fd=8):                                   │◄─┐                      │     │
│ │ │ • state = SENDING_RESPONSE                      │  │                      │     │
│ │ │ • request_buffer = "POST /upload HTTP/1.1..."  │  │                       │     │
│ │ │ • response_buffer = "HTTP/1.1 200 OK..."       │  │                       │     │
│ │ │ • bytes_sent = 150                              │  │                      │     │
│ │ └─────────────────────────────────────────────────┘  │                      │     │
│ │                                                      │                      │     │
│ │ std::map<int, ServerConfig> clientToServerMap:       │                      │     │
│ │ ┌────────┬──────────────────────────────────────┐    │                      │     │
│ │ │   6    │ ServerConfig{port=8080, root="/www"} │    │                      │     │
│ │ │   8    │ ServerConfig{port=8080, root="/www"} │    │                      │     │
│ │ └────────┴──────────────────────────────────────┘    │                      │     │
│ └─────────────────────────────────────────────────────────────────────────────┘     │
│                                                                                     │
│ ┌─────────────────────────────────────────────────────────────────────────────┐     │
│ │                        KERNEL SPACE                                         │     │
│ │                                                                             │     │
│ │ epoll instance (fd=4):                                                      │     │
│ │ ┌─────────────────────────────────────────────────────────────────────┐     │     │
│ │ │ Monitored FDs:                                                      │     │     │
│ │ │ fd=3  (server) → EPOLLIN                                            │     │     │
│ │ │ fd=6  (client) → EPOLLIN | EPOLLET                                  │     │     │
│ │ │ fd=7  (server) → EPOLLIN                                            │     │     │
│ │ │ fd=8  (client) → EPOLLOUT | EPOLLET                                 │     │     │
│ │ │ fd=11 (server) → EPOLLIN                                            │     │     │
│ │ └─────────────────────────────────────────────────────────────────────┘     │     │
│ │                                                                             │     │
│ │ Socket Buffers:                                                             │     │
│ │ ┌─────────────────────────────────────────────────────────────────────┐     │     │
│ │ │ fd=6 recv buffer: [HTTP request data...]                            │     │     │
│ │ │ fd=6 send buffer: [empty]                                           │     │     │
│ │ │ fd=8 recv buffer: [empty]                                           │     │     │
│ │ │ fd=8 send buffer: [HTTP response data...]                           │     │     │
│ │ └─────────────────────────────────────────────────────────────────────┘     │     │
│ └─────────────────────────────────────────────────────────────────────────────┘     │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## 5. Performance & Scalability Considerations

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         PERFORMANCE CHARACTERISTICS                                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│ Concurrency Model: Event-Driven (Single-Threaded)                                   │
│                                                                                     │
│ ┌─────────────────────────────────────────────────────────────────────────────┐     │
│ │                    TRADITIONAL vs WEBSERV                                   │     │
│ │                                                                             │     │
│ │ Traditional (Thread-per-Client):                                            │     │
│ │ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐                             │     │
│ │ │Thread 1 │ │Thread 2 │ │Thread 3 │ │Thread N │                             │     │
│ │ │Client A │ │Client B │ │Client C │ │Client Z │                             │     │
│ │ └─────────┘ └─────────┘ └─────────┘ └─────────┘                             │     │
│ │ Memory: ~8MB per thread * N threads = High memory usage                     │     │
│ │ Context Switching: Expensive                                                │     │
│ │ Scalability: Limited by thread limits (~thousands)                          │     │
│ │                                                                             │     │
│ │ Webserv (Event-Driven):                                                     │     │
│ │ ┌─────────────────────────────────────────────────────────────────────┐     │     │
│ │ │                    Single Event Loop                                │     │     │
│ │ │ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐                     │     │     │
│ │ │ │Client A │ │Client B │ │Client C │ │Client Z │                     │     │     │
│ │ │ │ State   │ │ State   │ │ State   │ │ State   │                     │     │     │
│ │ │ └─────────┘ └─────────┘ └─────────┘ └─────────┘                     │     │     │
│ │ └─────────────────────────────────────────────────────────────────────┘     │     │
│ │ Memory: ~KB per client state                                                │     │
│ │ Context Switching: None                                                     │     │
│ │ Scalability: Limited by file descriptor limits (~65k+)                      │     │
│ └─────────────────────────────────────────────────────────────────────────────┘     │
│                                                                                     │
│ Bottlenecks & Optimizations:                                                        │
│                                                                                     │
│ 1. File I/O Operations:                                                             │
│    • Current: Blocking file reads                                                   │
│    • Optimization: Memory mapping (mmap) for static files                           │
│                                                                                     │
│ 2. String Operations:                                                               │
│    • Current: Multiple string copies during parsing                                 │
│    • Optimization: Zero-copy parsing with string_view                               │
│                                                                                     │
│ 3. Memory Allocation:                                                               │
│    • Current: Dynamic allocation for each client                                    │
│    • Optimization: Object pooling for Client objects                                │
│                                                                                     │
│ 4. epoll Event Handling:                                                            │
│    • Current: Process events one by one                                             │
│    • Optimization: Batch processing of similar events                               │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

This comprehensive flow map demonstrates the complete lifecycle of an HTTP request in the Webserv architecture, from initial connection to final cleanup, including all major data structures, state transitions, and system interactions.
