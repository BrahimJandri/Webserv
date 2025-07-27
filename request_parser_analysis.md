# Request Parser Implementation Analysis

## 1. Request Parser Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           requestParser Class                                   │
│                                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │  Core Data      │    │  Parsing Engine │    │  Validation     │            │
│  │                 │    │                 │    │                 │            │
│  │ • _method       │    │ • parseRequest()│    │ • Header        │            │
│  │ • _path         │    │ • Header Parser │    │   Validation    │            │
│  │ • _httpVersion  │    │ • Body Parser   │    │ • Method        │            │
│  │ • _headers      │    │ • Line Parser   │    │   Validation    │            │
│  │ • _body         │    │                 │    │ • Path          │            │
│  └─────────────────┘    └─────────────────┘    │   Validation    │            │
│           │                       │             └─────────────────┘            │
│           └───────────────────────┼───────────────────────────────────────────┐│
│                                   │                                           ││
│                           ┌─────────────────┐                                 ││
│                           │ HTTP Request    │                                 ││
│                           │ Data Structure  │                                 ││
│                           └─────────────────┘                                 ││
│                                                                               ││
│  Getters:                                                                     ││
│  ┌─────────────────────────────────────────────────────────────────────────┐ ││
│  │ getMethod(), getPath(), getHttpVersion(), getHeaders(), getBody()       │ ││
│  └─────────────────────────────────────────────────────────────────────────┘ ││
└─────────────────────────────────────────────────────────────────────────────────┘
```

## 2. HTTP Request Structure and Parsing

### Raw HTTP Request Format:
```
Raw HTTP Request Structure:
┌─────────────────────────────────────────────────────────────┐
│ Request Line:                                               │
│ GET /index.html HTTP/1.1\r\n                              │
├─────────────────────────────────────────────────────────────┤
│ Headers:                                                    │
│ Host: localhost:8080\r\n                                   │
│ User-Agent: Mozilla/5.0...\r\n                            │
│ Accept: text/html,application/xhtml+xml...\r\n            │
│ Connection: keep-alive\r\n                                 │
│ Content-Type: application/x-www-form-urlencoded\r\n        │
│ Content-Length: 27\r\n                                     │
├─────────────────────────────────────────────────────────────┤
│ Empty Line (Headers End):                                  │
│ \r\n                                                       │
├─────────────────────────────────────────────────────────────┤
│ Body (for POST/PUT requests):                              │
│ name=John+Doe&email=john%40example.com                     │
└─────────────────────────────────────────────────────────────┘
```

### Request Parser Data Structure:
```
requestParser Object Memory Layout:
┌─────────────────────────────────────────────────────────────┐
│ requestParser instance                                      │
│                                                             │
│ ┌─────────────────┐    ┌─────────────────┐                │
│ │ _method:        │    │ _path:          │                │
│ │ "POST"          │    │ "/submit"       │                │
│ └─────────────────┘    └─────────────────┘                │
│                                                             │
│ ┌─────────────────┐    ┌─────────────────┐                │
│ │ _httpVersion:   │    │ _body:          │                │
│ │ "HTTP/1.1"      │    │ "name=John..."  │                │
│ └─────────────────┘    └─────────────────┘                │
│                                                             │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ _headers (std::map<std::string, std::string>):          │ │
│ │ ┌─────────────────┐ ┌─────────────────┐                │ │
│ │ │ "Host"          │→│ "localhost:8080"│                │ │
│ │ │ "User-Agent"    │→│ "Mozilla/5.0..."│                │ │
│ │ │ "Content-Type"  │→│ "application/..." │               │ │
│ │ │ "Content-Length"│→│ "27"            │                │ │
│ │ │ "Cookie"        │→│ "sessionid=abc"  │                │ │
│ │ └─────────────────┘ └─────────────────┘                │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## 3. Request Parsing Algorithm Deep Dive

### parseRequest() Method Flow:
```
parseRequest() Execution Flow:

Input: Raw HTTP Request String
         │
         ▼
┌─────────────────┐
│ Clear previous  │ ──► _method.clear()
│ state           │     _path.clear()
│                 │     _httpVersion.clear()
│                 │     _headers.clear()
│                 │     _body.clear()
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Find header/    │ ──► │ size_t pos = rawRequest.find(       │
│ body separator  │     │              "\r\n\r\n");          │
│ "\r\n\r\n"      │     └─────────────────────────────────────┘
└─────────────────┘
         │
    ┌────┴────┐
   FOUND      NOT FOUND
    │          │
    ▼          ▼
┌─────────┐ ┌─────────────────┐
│Headers  │ │Headers only     │
│+ Body   │ │(incomplete or   │
│Mode     │ │no body)         │
└─────────┘ └─────────────────┘
    │          │
    ▼          ▼
┌─────────────────────────────────────────────────────────────┐
│ Split into header_part and body_part                       │
│ header_part = rawRequest.substr(0, header_end_pos)         │
│ _body = rawRequest.substr(header_end_pos + 4)              │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Parse Request   │ ──► │ std::istringstream line_iss(line);  │
│ Line            │     │ line_iss >> _method >> _path >>     │
│                 │     │             _httpVersion;           │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Parse Headers   │ ──► │ while (std::getline(iss, line)) {   │
│ Line by Line    │     │   Parse "Key: Value" pairs          │
│                 │     │   Store in _headers map             │
│                 │     │ }                                   │
└─────────────────┘     └─────────────────────────────────────┘
```

### Header Parsing Algorithm:
```
Header Line Parsing Process:

Input: "Content-Type: application/json\r\n"
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Remove trailing │ ──► │ if (line[line.length()-1] == '\r')  │
│ \r character    │     │   line = line.substr(0, length-1)   │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Find colon      │ ──► │ size_t colon_pos = line.find(':');  │
│ separator       │     └─────────────────────────────────────┘
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Split key and   │ ──► │ key = line.substr(0, colon_pos)     │
│ value           │     │ value = line.substr(colon_pos + 1)  │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Trim whitespace │ ──► │ key.erase(0, key.find_first_not_of  │
│ from key/value  │     │          (" \t"));                 │
│                 │     │ key.erase(key.find_last_not_of      │
│                 │     │          (" \t") + 1);             │
│                 │     │ (Same for value)                    │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Store in        │ ──► │ _headers[key] = value;              │
│ headers map     │     └─────────────────────────────────────┘
└─────────────────┘

Result: _headers["Content-Type"] = "application/json"
```

## 4. Request Validation and Error Handling

### Request Completeness Detection:
```
Request Completeness Algorithm:

┌─────────────────────────────────────────────────────────────┐
│ Determine if HTTP request is complete:                      │
│                                                             │
│ 1. Headers complete? (found "\r\n\r\n")                   │
│ 2. If POST/PUT: Body length matches Content-Length?        │
│ 3. If chunked: All chunks received?                        │
└─────────────────────────────────────────────────────────────┘

Client::processRequest() Integration:
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Accumulate data │ ──► │ _buffer += new_data                 │
│ in buffer       │     └─────────────────────────────────────┘
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Check for       │ ──► │ if (_buffer.find("\r\n\r\n")       │
│ complete headers│     │     != std::string::npos)           │
└─────────────────┘     └─────────────────────────────────────┘
         │
    ┌────┴────┐
  FOUND      NOT FOUND
    │          │
    ▼          ▼
┌─────────┐ ┌─────────────────┐
│Check    │ │Return false     │
│Content- │ │(need more data) │
│Length   │ └─────────────────┘
└─────────┘
    │
    ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Body length     │ ──► │ actual_body_length ==               │
│ validation      │     │ expected_content_length?            │
└─────────────────┘     └─────────────────────────────────────┘
         │
    ┌────┴────┐
  MATCH     MISMATCH
    │          │
    ▼          ▼
┌─────────┐ ┌─────────────────┐
│Parse    │ │Return false     │
│request  │ │(incomplete)     │
│Return   │ └─────────────────┘
│true     │
└─────────┘
```

## 5. Memory Management and Performance

### String Operations Performance:
```
Memory Usage During Request Parsing:

┌─────────────────────────────────────────────────────────────┐
│                    Memory Timeline                          │
│                                                             │
│ Memory │                                                    │
│ Usage  │      ╭─╮                                          │
│   ▲    │     ╱   ╲                                         │
│   │    │    ╱     ╲ ← Peak: Raw request + parsed objects   │
│   │    │   ╱       ╲                                       │
│   │    │  ╱         ╲                                      │
│   │    │ ╱           ╲                                     │
│   │    │╱             ╲____________________________       │
│   0    └─────────────────╲                           ╲     │
│        Raw    Parse    Process                     Clean    │
│       Request Request  Request                      Up      │
└─────────────────────────────────────────────────────────────┘

Memory Allocation Points:
1. Client buffer accumulation (_buffer += data)
2. Header/body separation (substr operations)
3. Header parsing (map insertions)
4. String trimming operations

Memory Optimization Techniques:
• Use references for getters (const std::string&)
• Minimize string copying with substr()
• Clear previous state before parsing new request
• Use map for O(log n) header lookup
```

### Error Handling Scenarios:
```
Request Parsing Error Scenarios:

┌─────────────────────────────────────────────────────────────┐
│ Error Type              │ Detection               │ Action  │
├─────────────────────────────────────────────────────────────┤
│ Malformed request line  │ Missing method/path/ver │ 400     │
│ Missing headers         │ No \r\n\r\n found      │ Wait    │
│ Invalid header format   │ No colon in header line │ Ignore  │
│ Content-Length mismatch │ Body size != header val │ Wait    │
│ Oversized request       │ Buffer > max size       │ 413     │
│ Timeout                 │ No data for X seconds   │ Close   │
└─────────────────────────────────────────────────────────────┘

Error Flow Example:
┌─────────────────┐
│ Parse request   │
│ line fails      │
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Set _method to  │ ──► │ Allows Server::prepareResponse()    │
│ empty string    │     │ to detect invalid request           │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Server returns  │ ──► │ send_error_response(client_fd,      │
│ 400 Bad Request │     │                     400, ...)      │
└─────────────────┘     └─────────────────────────────────────┘
```

## 6. Integration with Server Architecture

### Request Processing Pipeline:
```
Request Flow Through Server Components:

┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ TCP Data        │ ──►│ Client::        │ ──►│ requestParser:: │
│ Received        │    │ _buffer         │    │ parseRequest()  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ EPOLLIN event   │    │ Accumulate      │    │ Parse HTTP      │
│ triggers        │    │ request data    │    │ components      │
│ handleClientRead│    │ until complete  │    │ into object     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
                                ▼                       ▼
                       ┌─────────────────┐    ┌─────────────────┐
                       │ Client::        │    │ Server::        │
                       │ processRequest()│ ──►│ prepareResponse │
                       │ returns true    │    │ (requestParser) │
                       └─────────────────┘    └─────────────────┘
```

### Thread Safety Considerations:
```
Thread Safety in Request Processing:

┌─────────────────────────────────────────────────────────────┐
│ Single-Threaded Design Benefits:                            │
│                                                             │
│ • No race conditions in request parsing                     │
│ • No mutex locks needed for requestParser objects          │
│ • Simplified memory management                              │
│ • Deterministic execution order                             │
│                                                             │
│ Potential Issues with Multi-Threading:                      │
│                                                             │
│ • Shared static MIME type maps                             │
│ • File descriptor access conflicts                          │
│ • Global error handling state                               │
└─────────────────────────────────────────────────────────────┘

Per-Client Request Objects:
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Client A        │    │ Client B        │    │ Client C        │
│ requestParser   │    │ requestParser   │    │ requestParser   │
│ (independent)   │    │ (independent)   │    │ (independent)   │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                         No shared state
```

This comprehensive analysis shows how the requestParser class efficiently parses HTTP requests while maintaining clean separation of concerns and robust error handling within the server's multiplexing architecture.
