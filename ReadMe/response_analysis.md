# Response Class and HTTP Methods Analysis

## 1. Response Class Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                               Response Class                                    │
│                                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │  Core Data      │    │  HTTP Methods   │    │  Utility Methods│            │
│  │                 │    │                 │    │                 │            │
│  │ • statusCode    │    │ • buildGet()    │    │ • getContentType│            │
│  │ • statusMessage │    │ • buildPost()   │    │ • fileExists()  │            │
│  │ • _headers      │    │ • buildDelete() │    │ • toString()    │            │
│  │ • _body         │    │                 │    │ • urlDecode()   │            │
│  │ • _httpVersion  │    │                 │    │                 │            │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘            │
│           │                       │                       │                    │
│           └───────────────────────┼───────────────────────┘                    │
│                                   │                                            │
│                           ┌─────────────────┐                                  │
│                           │ HTTP Response   │                                  │
│                           │ String Builder  │                                  │
│                           └─────────────────┘                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## 2. Response Class Core Structure

### Data Members Analysis:
```cpp
class Response {
private:
    int statusCode;                              // HTTP status code (200, 404, etc.)
    std::string statusMessage;                   // HTTP status text ("OK", "Not Found")
    std::map<std::string, std::string> _headers; // HTTP headers (Content-Type, etc.)
    std::string _body;                           // Response body content
    std::string _httpVersion;                    // HTTP version ("HTTP/1.1")
};
```

### Memory Layout Visualization:
```
Response Object in Memory:
┌─────────────────────────────────────────────────────────────┐
│ Response Instance                                           │
│ ┌─────────────────┐  ┌─────────────────┐                   │
│ │ statusCode: 200 │  │statusMessage:   │                   │
│ │ (4 bytes)       │  │ "OK"            │                   │
│ └─────────────────┘  └─────────────────┘                   │
│                                                             │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ _headers (std::map):                                    │ │
│ │ ┌─────────────────┐ ┌─────────────────┐                │ │
│ │ │"Content-Type"   │→│"text/html"      │                │ │
│ │ │"Content-Length" │→│"1024"           │                │ │
│ │ │"Server"         │→│"Webserv/1.0"    │                │ │
│ │ │"Connection"     │→│"close"          │                │ │
│ │ └─────────────────┘ └─────────────────┘                │ │
│ └─────────────────────────────────────────────────────────┘ │
│                                                             │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ _body: "<!DOCTYPE html><html>...</html>"                │ │
│ │ (Variable size string containing response content)      │ │
│ └─────────────────────────────────────────────────────────┘ │
│                                                             │
│ ┌─────────────────┐                                        │
│ │ _httpVersion:   │                                        │
│ │ "HTTP/1.1"      │                                        │
│ └─────────────────┘                                        │
└─────────────────────────────────────────────────────────────┘
```

## 3. HTTP Response Construction Process

### toString() Method Flow:
```
toString() Method Execution:
         │
         ▼
┌─────────────────┐
│ Create          │ ──► std::ostringstream oss;
│ string stream   │
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Build Status    │ ──► │ "HTTP/1.1 200 OK\r\n"              │
│ Line            │     └─────────────────────────────────────┘
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Add Headers     │ ──► │ "Content-Type: text/html\r\n"       │
│ (loop through   │     │ "Content-Length: 1024\r\n"         │
│ _headers map)   │     │ "Server: Webserv/1.0\r\n"          │
│                 │     │ "Connection: close\r\n"            │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Add Content-    │ ──► │ "Content-Length: 1024\r\n"         │
│ Length if       │     │ (if not already present)           │
│ missing         │     └─────────────────────────────────────┘
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Add blank line  │ ──► │ "\r\n"                              │
│ (end of headers)│     └─────────────────────────────────────┘
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Add body        │ ──► │ "<!DOCTYPE html><html>...</html>"   │
│ content         │     └─────────────────────────────────────┘
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Return complete │
│ HTTP response   │
│ string          │
└─────────────────┘

Final HTTP Response Format:
┌─────────────────────────────────────────────────────────────┐
│ HTTP/1.1 200 OK\r\n                                        │
│ Content-Type: text/html\r\n                                │
│ Content-Length: 1024\r\n                                   │
│ Server: Webserv/1.0\r\n                                    │
│ Connection: close\r\n                                      │
│ \r\n                                                       │
│ <!DOCTYPE html>                                            │
│ <html>                                                     │
│ <head><title>Page Title</title></head>                     │
│ <body>                                                     │
│ <h1>Hello World!</h1>                                     │
│ </body>                                                    │
│ </html>                                                    │
└─────────────────────────────────────────────────────────────┘
```

## 4. Content-Type Detection System

### MIME Type Mapping:
```cpp
static std::map<std::string, std::string> mimeTypes;

// Content-Type determination flow:
File Extension → MIME Type Lookup → Content-Type Header

┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ input.html      │ ──►│ .html          │ ──►│ text/html       │
└─────────────────┘    └─────────────────┘    └─────────────────┘

┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ style.css       │ ──►│ .css           │ ──►│ text/css        │
└─────────────────┘    └─────────────────┘    └─────────────────┘

┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ image.jpg       │ ──►│ .jpg           │ ──►│ image/jpeg      │
└─────────────────┘    └─────────────────┘    └─────────────────┘

┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ unknown.xyz     │ ──►│ .xyz (unknown) │ ──►│ application/    │
│                 │    │                │    │ octet-stream    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### MIME Types Table:
```
Supported MIME Types:
┌─────────────────┬─────────────────────────────┐
│ Extension       │ MIME Type                   │
├─────────────────┼─────────────────────────────┤
│ .html           │ text/html                   │
│ .css            │ text/css                    │
│ .js             │ application/javascript      │
│ .txt            │ text/plain                  │
│ .jpg/.jpeg      │ image/jpeg                  │
│ .png            │ image/png                   │
│ .gif            │ image/gif                   │
│ .ico            │ image/x-icon                │
│ (unknown)       │ application/octet-stream    │
└─────────────────┴─────────────────────────────┘
```
