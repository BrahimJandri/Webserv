# handleConnections() Function Analysis

## Function Overview
```cpp
void Server::handleConnections()
{
    signal(SIGINT, sighandler);    // Line 1
    signal(SIGQUIT, sighandler);   // Line 2

    struct epoll_event events[MAX_EVENTS];  // Line 3

    while (!_turnoff)  // Line 4
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);  // Line 5
        
        if (num_events == -1)  // Line 6
        {
            if (errno == EINTR)  // Line 7
            {
                continue;  // Line 8
            }
            perror("epoll_wait");  // Line 9
            break;  // Line 10
        }

        for (int i = 0; i < num_events; ++i)  // Line 11
        {
            int fd = events[i].data.fd;  // Line 12

            if (server_fds.find(fd) != server_fds.end())  // Line 13
            {
                acceptNewConnection(fd);  // Line 14
            }
            else if (events[i].events & EPOLLIN)  // Line 15
            {
                handleClientRead(fd);  // Line 16
            }
            else if (events[i].events & EPOLLOUT)  // Line 17
            {
                handleClientWrite(fd);  // Line 18
            }
        }
    }
}
```

## Detailed Line-by-Line Explanation

### Signal Handling Setup (Lines 1-2)
```cpp
signal(SIGINT, sighandler);    // Ctrl+C
signal(SIGQUIT, sighandler);   // Ctrl+\
```
**Purpose**: Set up signal handlers for graceful shutdown
- SIGINT: Interrupt signal (Ctrl+C)
- SIGQUIT: Quit signal (Ctrl+\)
- When received, these signals set `_turnoff = 1` to exit the main loop

### Event Array Declaration (Line 3)
```cpp
struct epoll_event events[MAX_EVENTS];  // MAX_EVENTS = 64
```
**Purpose**: Create an array to store events returned by epoll_wait()
- Each element contains information about a ready file descriptor
- MAX_EVENTS limits how many events we can process per iteration

### Main Event Loop (Line 4)
```cpp
while (!_turnoff)
```
**Purpose**: Continue processing events until shutdown signal received
- `_turnoff` is a volatile sig_atomic_t variable
- Set to 1 by signal handler for graceful shutdown

### epoll_wait() Call (Line 5)
```cpp
int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
```
**Purpose**: Block until events are available
- `epoll_fd`: The epoll instance created in constructor
- `events`: Array to fill with ready events
- `MAX_EVENTS`: Maximum events to return
- `-1`: Block indefinitely (timeout = -1)

**Return Value**: Number of ready file descriptors, or -1 on error

### Error Handling (Lines 6-10)
```cpp
if (num_events == -1)
{
    if (errno == EINTR)  // Interrupted by signal
    {
        continue;  // Not a real error, continue loop
    }
    perror("epoll_wait");  // Real error occurred
    break;  // Exit loop
}
```
**Purpose**: Handle epoll_wait() errors gracefully
- EINTR: System call interrupted by signal (expected during shutdown)
- Other errors: Log and exit

### Event Processing Loop (Lines 11-19)
```cpp
for (int i = 0; i < num_events; ++i)
{
    int fd = events[i].data.fd;  // Get file descriptor
    
    // Check if it's a server socket (listening socket)
    if (server_fds.find(fd) != server_fds.end())
    {
        acceptNewConnection(fd);  // New client connecting
    }
    // Check if client socket ready for reading
    else if (events[i].events & EPOLLIN)
    {
        handleClientRead(fd);  // Read data from client
    }
    // Check if client socket ready for writing
    else if (events[i].events & EPOLLOUT)
    {
        handleClientWrite(fd);  // Send data to client
    }
}
```

## Event Types Explanation

### EPOLLIN
- **Meaning**: File descriptor is ready for reading
- **For Server Socket**: New connection available
- **For Client Socket**: Data available to read

### EPOLLOUT
- **Meaning**: File descriptor is ready for writing
- **For Client Socket**: Can send data without blocking

### EPOLLET (Edge-Triggered)
- **Used in**: Client socket registration
- **Meaning**: Only notify when state changes (not level-triggered)
- **Advantage**: More efficient, avoids repeated notifications

## Flow Diagram

```
Start
  │
  ▼
┌─────────────────┐
│ Setup Signals   │
└─────────────────┘
  │
  ▼
┌─────────────────┐
│ Create events[] │
└─────────────────┘
  │
  ▼
┌─────────────────┐    No     ┌─────────────────┐
│ !_turnoff?      │ ────────▶ │ Exit Function   │
└─────────────────┘           └─────────────────┘
  │ Yes
  ▼
┌─────────────────┐
│ epoll_wait()    │ ◄─────┐
└─────────────────┘       │
  │                       │
  ▼                       │
┌─────────────────┐       │
│ Error check     │       │
└─────────────────┘       │
  │                       │
  ▼                       │
┌─────────────────┐       │
│ Process events  │       │
│ for each fd:    │       │
│ - Server? →     │       │
│   Accept new    │       │
│ - EPOLLIN? →    │       │
│   Read data     │       │
│ - EPOLLOUT? →   │       │
│   Write data    │ ──────┘
└─────────────────┘
```
