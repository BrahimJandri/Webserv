# HTTP Request-Response Lifecycle Visualization

## 1. Complete HTTP Transaction Flow

```
                          Complete HTTP Request-Response Cycle
                                        
                    ┌─────────────────────────────────────────────────┐
                    │                  Internet                       │
                    └─────────────────┬───────────────────────────────┘
                                      │
┌─────────────────┐                   │                   ┌─────────────────┐
│    Client       │                   │                   │   Web Server    │
│   (Browser)     │                   │                   │   (Webserv)     │
│                 │                   │                   │                 │
│ 1. User clicks  │────── HTTP ──────┼──────────────────►│ 2. epoll_wait() │
│    link/submits │       Request     │                   │    receives     │
│    form         │                   │                   │    EPOLLIN      │
│                 │                   │                   │                 │
│ 8. Browser      │◄───── HTTP ──────┼───────────────────│ 7. Send HTTP    │
│    renders      │       Response    │                   │    response     │
│    page/file    │                   │                   │    via write()  │
└─────────────────┘                   │                   └─────────────────┘
                                      │                            │
                                      │                            │
                          ┌───────────┴───────────┐                │
                          │    Request Flow       │                │
                          │   (Steps 3-6)        │                │
                          └───────────────────────┘                │
                                      │                            │
                                      ▼                            │
                    ┌─────────────────────────────────────────────────┐
                    │              Server Internal Flow              │
                    │                                                 │
                    │ 3. handleClientRead() ──► 4. parseRequest()    │
                    │           │                       │             │
                    │           ▼                       ▼             │
                    │ 5. prepareResponse() ──► 6. Build Method       │
                    │    • GET: buildGetResponse()                   │
                    │    • POST: buildPostResponse()                 │
                    │    • DELETE: buildDeleteResponse()             │
                    └─────────────────────────────────────────────────┘
```

## 2. Detailed Request Processing Pipeline

### Phase 1: Request Reception and Parsing
```
Request Reception Flow:

TCP Connection Established
         │
         ▼
┌─────────────────┐
│ EPOLLIN event   │ ──► epoll_wait() returns client_fd
│ on client_fd    │
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ handleClientRead│ ──► │ Loop: read() until EAGAIN           │
│()               │     │ Accumulate data in Client::_buffer  │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Client::        │ ──► │ Parse HTTP headers                  │
│ processRequest()│     │ Extract Content-Length              │
│                 │     │ Check if body complete              │
└─────────────────┘     └─────────────────────────────────────┘
         │
    ┌────┴────┐
  COMPLETE   INCOMPLETE
    │          │
    ▼          ▼
┌─────────┐ ┌─────────────────┐
│Continue │ │Wait for more    │
│to       │ │data (return to  │
│Response │ │epoll loop)      │
│Phase    │ └─────────────────┘
└─────────┘
```

### Phase 2: Request Object Structure
```
requestParser Object Structure After Parsing:

┌─────────────────────────────────────────────────────────────┐
│ requestParser instance                                      │
│                                                             │
│ ┌─────────────────┐    ┌─────────────────┐                │
│ │ _method: "POST" │    │ _path: "/upload"│                │
│ └─────────────────┘    └─────────────────┘                │
│                                                             │
│ ┌─────────────────┐    ┌─────────────────┐                │
│ │ _httpVersion:   │    │ _headers:       │                │
│ │ "HTTP/1.1"      │    │ map<str,str>    │                │
│ └─────────────────┘    └─────────────────┘                │
│                               │                            │
│                               ▼                            │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ Headers Map:                                            │ │
│ │ "Host" → "localhost:8080"                               │ │
│ │ "Content-Type" → "multipart/form-data; boundary=xyz"    │ │
│ │ "Content-Length" → "1048576"                            │ │
│ │ "User-Agent" → "Mozilla/5.0..."                        │ │
│ │ "Accept" → "text/html,application/xhtml+xml..."        │ │
│ └─────────────────────────────────────────────────────────┘ │
│                                                             │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ _body: (1MB of multipart form data)                     │ │
│ │ "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"          │ │
│ │ "Content-Disposition: form-data; name=\"file\"..."     │ │
│ │ [binary file data]                                      │ │
│ │ "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n"        │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Phase 3: Method Dispatch and Processing
```
HTTP Method Dispatch:

prepareResponse() receives requestParser object
                │
                ▼
┌─────────────────┐
│ Extract method  │ ──► std::string method = req.getMethod();
│ from request    │
└─────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────────────────┐
│ Method Routing Switch:                                      │
│                                                             │
│ if (method == "GET")                                        │
│     response = Response::buildGetResponse(...)              │
│ else if (method == "POST")                                  │
│     response = Response::buildPostResponse(...)             │
│ else if (method == "DELETE")                                │
│     response = Response::buildDeleteResponse(...)           │
│ else                                                        │
│     send_error_response(client_fd, 405, "Method Not Allowed")│
└─────────────────────────────────────────────────────────────┘
                │
                ▼
┌─────────────────┐
│ Method-specific │ ──► Detailed processing based on HTTP method
│ processing      │     and request content
└─────────────────┘
```

## 3. Response Generation Deep Dive

### Response Object Assembly
```
Response Construction Process:

Response Constructor Called
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Initialize      │ ──► │ statusCode = 200                    │
│ default values  │     │ statusMessage = "OK"                │
│                 │     │ _httpVersion = "HTTP/1.1"           │
│                 │     │ Add default headers:                │
│                 │     │   "Server: Webserv/1.0"            │
│                 │     │   "Connection: close"               │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Method-specific │ ──► │ GET: Read file, set Content-Type    │
│ processing      │     │ POST: Process form, save data       │
│                 │     │ DELETE: Remove file/directory       │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Add Content-    │ ──► │ Calculate body size                 │
│ Length header   │     │ headers["Content-Length"] = size    │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Convert to      │ ──► │ response.toString()                 │
│ HTTP string     │     │ Returns complete HTTP response      │
└─────────────────┘     └─────────────────────────────────────┘
```

### Response String Building Process
```
toString() Method Execution Detail:

std::ostringstream oss;
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ Status Line Construction:                                   │
│ oss << _httpVersion << " " << statusCode << " "             │
│     << statusMessage << "\r\n";                            │
│                                                             │
│ Result: "HTTP/1.1 200 OK\r\n"                             │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ Headers Loop:                                               │
│ for (map<string,string>::const_iterator it = _headers.begin(); │
│      it != _headers.end(); ++it) {                         │
│     oss << it->first << ": " << it->second << "\r\n";      │
│ }                                                           │
│                                                             │
│ Result: "Content-Type: text/html\r\n"                      │
│         "Content-Length: 1024\r\n"                         │
│         "Server: Webserv/1.0\r\n"                          │
│         "Connection: close\r\n"                             │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ Content-Length Auto-Addition:                               │
│ if (_body.length() > 0 &&                                  │
│     _headers.find("Content-Length") == _headers.end()) {   │
│     oss << "Content-Length: " << _body.length() << "\r\n"; │
│ }                                                           │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ End of Headers:                                             │
│ oss << "\r\n";                                             │
│                                                             │
│ Result: "\r\n"  (blank line separating headers from body)  │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ Body Addition:                                              │
│ oss << _body;                                               │
│                                                             │
│ Result: Complete HTML/JSON/file content                    │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ Return Complete HTTP Response:                              │
│ return oss.str();                                           │
└─────────────────────────────────────────────────────────────┘
```

## 4. Error Handling and Edge Cases

### Error Response Generation Pipeline
```
Error Condition Detection:
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Error Context   │ ──► │ • File not found (404)              │
│ Analysis        │     │ • Permission denied (403)           │
│                 │     │ • Method not allowed (405)          │
│                 │     │ • Server error (500)                │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Status Code     │ ──► │ Map error type to HTTP status code  │
│ Determination   │     │ 404, 403, 405, 500, etc.           │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Custom Error    │ ──► │ Check serverConfig.error_pages      │
│ Page Lookup     │     │ for custom error page paths        │
└─────────────────┘     └─────────────────────────────────────┘
         │
    ┌────┴────┐
  FOUND      NOT FOUND
    │          │
    ▼          ▼
┌─────────┐ ┌─────────────────┐
│Load     │ │Use built-in     │
│custom   │ │default error    │
│error    │ │page template    │
│page     │ │                 │
└─────────┘ └─────────────────┘
    │          │
    └────┬─────┘
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Generate Error  │ ──► │ Create complete HTTP error response │
│ Response        │     │ with proper status and headers      │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Send via        │ ──► │ send_error_response() directly      │
│ socket write()  │     │ to client file descriptor          │
└─────────────────┘     └─────────────────────────────────────┘
```

### Memory Management Throughout Lifecycle
```
Memory Usage Pattern During HTTP Transaction:

┌─────────────────────────────────────────────────────────────┐
│                    Memory Usage Timeline                    │
│                                                             │
│ Memory │                                                    │
│ Usage  │     ╭─╮                                           │
│   ▲    │    ╱   ╲                                          │
│   │    │   ╱     ╲ ← Peak: Request + Response in memory    │
│   │    │  ╱       ╲                                        │
│   │    │ ╱         ╲                                       │
│   │    │╱           ╲                                      │
│   │    └─────────────╲___________________________________  │
│   0    │              ╲ ← Cleanup after response sent     │
│        │               ╲                                   │
│        └─────────────────╲─────────────────────────────► Time
│         Request    Response  Client                        │
│         Received   Built    Disconnected                   │
└─────────────────────────────────────────────────────────────┘

Memory Allocation Points:
1. Client object creation (acceptNewConnection)
2. Request buffer accumulation (handleClientRead)  
3. Request parsing (requestParser object)
4. Response building (Response object + file content)
5. Response string conversion (toString())

Memory Deallocation Points:
1. Response sent (handleClientWrite)
2. Client object destruction (closeClientConnection)
3. Automatic string/container cleanup (C++ destructors)
```

## 5. Performance Characteristics and Optimizations

### Processing Time Complexity
```
Operation Time Complexities:

┌─────────────────────────────────────────────────────────────┐
│ Operation                    │ Time Complexity              │
├─────────────────────────────────────────────────────────────┤
│ Client lookup                │ O(log n) - std::map          │
│ Header parsing               │ O(h) - h = number of headers │
│ Content-Type detection       │ O(1) - static map lookup     │
│ File reading                 │ O(f) - f = file size         │
│ Form data parsing            │ O(b) - b = body size         │
│ Multipart boundary splitting │ O(b × p) - p = parts count   │
│ Response string building     │ O(r) - r = response size     │
│ Directory recursion (DELETE) │ O(d × f) - d = depth, f = files│
└─────────────────────────────────────────────────────────────┘

Memory Complexity:
• Request parsing: O(r) where r = request size
• Response building: O(f) where f = file size  
• Form processing: O(b) where b = body size
• Error responses: O(1) - fixed small size
```

### Scalability Bottlenecks and Solutions
```
Potential Bottlenecks:

┌─────────────────────────────────────────────────────────────┐
│ Bottleneck              │ Impact              │ Mitigation   │
├─────────────────────────────────────────────────────────────┤
│ Large file uploads      │ Memory usage spike  │ Streaming    │
│ Many simultaneous       │ File descriptor     │ Connection   │
│ connections             │ exhaustion          │ pooling      │
│ Complex multipart       │ CPU-intensive       │ Async        │
│ parsing                 │ parsing             │ processing   │
│ Recursive directory     │ Long blocking       │ Depth limits │
│ deletion               │ operations          │ and timeouts │
│ Error page generation   │ I/O overhead        │ Template     │
│                        │                     │ caching      │
└─────────────────────────────────────────────────────────────┘
```

This comprehensive analysis demonstrates how the Response class and HTTP methods integrate with the multiplexing server architecture to handle diverse client requests efficiently while maintaining proper error handling and security measures.
