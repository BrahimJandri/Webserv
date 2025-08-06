# CGI (Common Gateway Interface) Implementation Analysis

## 1. CGI Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                              CGI System Architecture                            │
│                                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │ Request         │    │ CGI Handler     │    │ Script          │            │
│  │ Processing      │    │                 │    │ Execution       │            │
│  │                 │    │ • Environment   │    │                 │            │
│  │ • HTTP Request  │ ──►│   Preparation   │ ──►│ • Python/Shell  │            │
│  │ • Path Analysis │    │ • Process       │    │ • Interpreter   │            │
│  │ • CGI Detection │    │   Management    │    │ • I/O Handling  │            │
│  │                 │    │ • I/O Pipes     │    │                 │            │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘            │
│           │                       │                       │                    │
│           └───────────────────────┼───────────────────────┘                    │
│                                   │                                            │
│                           ┌─────────────────┐                                  │
│                           │ HTTP Response   │                                  │
│                           │ Builder         │                                  │
│                           └─────────────────┘                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## 2. CGI Process Lifecycle

### CGI Execution Flow:
```
CGI Execution Process:

HTTP Request → CGI Detection → Environment Setup → Process Creation → Response
     │              │                │                    │              │
     ▼              ▼                ▼                    ▼              ▼
┌─────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│GET/POST │  │File         │  │CGI          │  │fork() +     │  │Parse CGI    │
│/upload  │  │extension    │  │environment  │  │execve()     │  │output       │
│request  │  │matches      │  │variables    │  │script       │  │headers +    │
│         │  │configured   │  │setup        │  │execution    │  │body         │
│         │  │CGI mapping  │  │             │  │             │  │             │
└─────────┘  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘
```

### Process Creation and Management:
```
Process Creation Deep Dive:

Parent Process (Web Server)          Child Process (CGI Script)
        │                                       │
        ▼                                       ▼
┌─────────────────┐                    ┌─────────────────┐
│ Create pipes:   │                    │ Redirect I/O:   │
│ • stdin_pipe    │                    │ • stdin ←       │
│ • stdout_pipe   │                    │   stdin_pipe[0] │
└─────────────────┘                    │ • stdout →      │
        │                              │   stdout_pipe[1]│
        ▼                              └─────────────────┘
┌─────────────────┐                             │
│ fork() creates  │ ─────────────────────────── ▼
│ child process   │                    ┌─────────────────┐
└─────────────────┘                    │ Change directory│
        │                              │ to script path  │
        ▼                              └─────────────────┘
┌─────────────────┐                             │
│ Parent closes:  │                             ▼
│ • stdin_pipe[0] │                    ┌─────────────────┐
│ • stdout_pipe[1]│                    │ Setup environ-  │
└─────────────────┘                    │ ment variables  │
        │                              └─────────────────┘
        ▼                                       │
┌─────────────────┐                             ▼
│ Write POST data │                    ┌─────────────────┐
│ to stdin_pipe[1]│                    │ execve() to     │
└─────────────────┘                    │ replace process │
        │                              │ with script     │
        ▼                              └─────────────────┘
┌─────────────────┐                             │
│ Read script     │                             ▼
│ output from     │                    ┌─────────────────┐
│ stdout_pipe[0]  │                    │ Script executes │
└─────────────────┘                    │ and writes to   │
        │                              │ stdout          │
        ▼                              └─────────────────┘
┌─────────────────┐
│ waitpid() for   │
│ child to finish │
└─────────────────┘
```

## 3. CGI Environment Variables

### Environment Variable Preparation:
```cpp
std::map<std::string, std::string> CGIHandler::prepareCGIEnv(const requestParser &req)
```

### CGI Environment Variables Mapping:
```
CGI Standard Environment Variables:

┌─────────────────────────────────────────────────────────────┐
│ Variable Name        │ Source                │ Example      │
├─────────────────────────────────────────────────────────────┤
│ REQUEST_METHOD       │ req.getMethod()       │ "POST"       │
│ SCRIPT_NAME          │ req.getPath()         │ "/cgi/test"  │
│ SERVER_PROTOCOL      │ req.getHttpVersion()  │ "HTTP/1.1"   │
│ QUERY_STRING         │ URL after '?'         │ "id=123"     │
│ CONTENT_LENGTH       │ POST body size        │ "256"        │
│ CONTENT_TYPE         │ Request Content-Type  │ "text/plain" │
│ GATEWAY_INTERFACE    │ CGI version           │ "CGI/1.1"    │
│ SERVER_SOFTWARE      │ Server identifier     │ "Webserv/1.0"│
│ HTTP_*               │ HTTP headers          │ HTTP_HOST    │
└─────────────────────────────────────────────────────────────┘
```

### HTTP Header to CGI Environment Transformation:
```
Header Transformation Algorithm:

HTTP Header: "Content-Type: application/json"
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ 1. Extract header name: "Content-Type"                     │
└─────────────────────────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ 2. Transform to CGI format:                                │
│    • Replace '-' with '_'                                  │
│    • Convert to uppercase                                  │
│    • Add "HTTP_" prefix                                    │
└─────────────────────────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ 3. Result: "HTTP_CONTENT_TYPE"                             │
└─────────────────────────────────────────────────────────────┘

Transformation Function:
char to_cgi_char(char c) {
    if (c == '-') return '_';
    return std::toupper(static_cast<unsigned char>(c));
}

Examples:
"User-Agent"     → "HTTP_USER_AGENT"
"Accept-Encoding"→ "HTTP_ACCEPT_ENCODING"
"Cookie"         → "HTTP_COOKIE"
```

### Environment Array Building:
```
buildEnvArray() Memory Management:

Input: std::map<std::string, std::string> envVars
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ Allocate char** array:                                      │
│ char **env = new char*[envVars.size() + 1];                │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ For each environment variable:                              │
│ 1. Create "KEY=VALUE" string                               │
│ 2. Allocate char array for string                          │
│ 3. Copy string to allocated memory                         │
│ 4. Store pointer in env array                              │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│ Set last element to NULL:                                   │
│ env[envVars.size()] = 0;                                   │
└─────────────────────────────────────────────────────────────┘

Memory Layout:
┌─────────────────┐    ┌─────────────────────────────────────┐
│ env[0] ─────────┼───►│ "REQUEST_METHOD=POST\0"             │
├─────────────────┤    └─────────────────────────────────────┘
│ env[1] ─────────┼───►┌─────────────────────────────────────┐
├─────────────────┤    │ "CONTENT_TYPE=application/json\0"   │
│ env[2] ─────────┼───►└─────────────────────────────────────┘
├─────────────────┤    ┌─────────────────────────────────────┐
│ env[3] ─────────┼───►│ "CONTENT_LENGTH=256\0"              │
├─────────────────┤    └─────────────────────────────────────┘
│ env[4] = NULL   │
└─────────────────┘
```

## 4. I/O Pipeline Management

### Pipe Communication Architecture:
```
CGI I/O Pipeline:

Web Server Process                 CGI Script Process
┌─────────────────┐               ┌─────────────────┐
│                 │               │                 │
│ POST Data ─────►│ stdin_pipe[1] │──────────────────►│ STDIN          │
│                 │               │               │  │ (script input) │
│                 │               │               │  └─────────────────┘
│                 │               │               │            │
│                 │               │               │            ▼
│                 │               │               │  ┌─────────────────┐
│                 │               │               │  │ Script          │
│                 │               │               │  │ Processing      │
│                 │               │               │  └─────────────────┘
│                 │               │               │            │
│                 │               │               │            ▼
│ Response ◄──────│stdout_pipe[0] │◄──────────────────│ STDOUT         │
│ Data            │               │               │  │ (script output) │
└─────────────────┘               └─────────────────┘  └─────────────────┘

Pipe File Descriptors:
stdin_pipe[0]  ← Child reads from (STDIN)
stdin_pipe[1]  ← Parent writes to
stdout_pipe[0] ← Parent reads from  
stdout_pipe[1] ← Child writes to (STDOUT)
```

### Data Flow During CGI Execution:
```
CGI Data Flow Timeline:

Time: T1 - Pipe Creation
┌─────────────────┐    ┌─────────────────┐
│ pipe(stdin_pipe)│    │pipe(stdout_pipe)│
│ [0] [1]         │    │ [0] [1]         │
└─────────────────┘    └─────────────────┘

Time: T2 - Process Fork
┌─────────────────┐              ┌─────────────────┐
│ Parent Process  │              │ Child Process   │
│ • Closes [0]    │              │ • Closes [1]    │
│ • Keeps [1]     │              │ • Keeps [0]     │
│ for writing     │              │ for reading     │
└─────────────────┘              └─────────────────┘

Time: T3 - I/O Redirection (Child)
┌─────────────────┐
│ dup2(stdin_pipe[0], STDIN_FILENO)  │
│ dup2(stdout_pipe[1], STDOUT_FILENO)│
└─────────────────┘

Time: T4 - Data Transfer
┌─────────────────┐              ┌─────────────────┐
│ Parent writes   │ ────────────►│ Child reads     │
│ POST body to    │              │ from STDIN      │
│ stdin_pipe[1]   │              │                 │
└─────────────────┘              └─────────────────┘
         │                                │
         ▼                                ▼
┌─────────────────┐              ┌─────────────────┐
│ Parent reads    │ ◄────────────│ Child writes    │
│ response from   │              │ to STDOUT       │
│ stdout_pipe[0]  │              │                 │
└─────────────────┘              └─────────────────┘
```

## 5. CGI Response Processing

### CGI Output Format:
```
CGI Script Output Format:

┌─────────────────────────────────────────────────────────────┐
│ Headers Section:                                            │
│ Content-Type: text/html\n                                  │
│ Set-Cookie: sessionid=abc123; Path=/\n                     │
│ Custom-Header: custom-value\n                              │
├─────────────────────────────────────────────────────────────┤
│ Empty Line (Headers End):                                  │
│ \n\n  or  \r\n\r\n                                        │
├─────────────────────────────────────────────────────────────┤
│ Body Section:                                               │
│ <!DOCTYPE html>                                            │
│ <html>                                                     │
│ <head><title>CGI Response</title></head>                   │
│ <body>                                                     │
│ <h1>Hello from CGI!</h1>                                   │
│ </body>                                                    │
│ </html>                                                    │
└─────────────────────────────────────────────────────────────┘
```

### Response Parsing Algorithm:
```
CGI Response Parsing Flow:

CGI Script Output String
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Find header/    │ ──► │ pos = output.find("\r\n\r\n");     │
│ body separator  │     │ if (not found)                     │
│                 │     │   pos = output.find("\n\n");       │
└─────────────────┘     └─────────────────────────────────────┘
         │
    ┌────┴────┐
   FOUND      NOT FOUND
    │          │
    ▼          ▼
┌─────────┐ ┌─────────────────┐
│Split    │ │Treat entire     │
│headers  │ │output as body   │
│and body │ │(no headers)     │
└─────────┘ └─────────────────┘
    │          │
    ▼          ▼
┌─────────────────────────────────────────────────────────────┐
│ Parse Headers Line by Line:                                │
│                                                             │
│ headerStream >> line                                        │
│ while (getline(headerStream, line)) {                      │
│   if (line.empty()) break;                                 │
│   colon = line.find(':');                                  │
│   key = line.substr(0, colon);                             │
│   value = line.substr(colon + 1);                          │
│   trim_whitespace(key, value);                             │
│   response.addHeader(key, value);                          │
│ }                                                           │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Add default     │ ──► │ if (no Content-Type header)         │
│ headers if      │     │   addHeader("Content-Type",        │
│ missing         │     │             "text/html");          │
│                 │     │ addHeader("Content-Length",        │
│                 │     │           body.size());             │
└─────────────────┘     └─────────────────────────────────────┘
```

## 6. Security and Error Handling

### CGI Security Measures:
```
Security Considerations:

┌─────────────────────────────────────────────────────────────┐
│ Path Security:                                              │
│ • Change directory to script's directory                    │
│ • Prevent directory traversal attacks                       │
│ • Validate script exists and is executable                  │
│                                                             │
│ Process Security:                                           │
│ • Use fork() + execve() for clean process separation       │
│ • Close unnecessary file descriptors                        │
│ • Set proper environment variables only                     │
│                                                             │
│ Resource Limits:                                            │
│ • Timeout for script execution                              │
│ • Memory limits for child process                           │
│ • Maximum output size limits                                │
│                                                             │
│ Input Validation:                                           │
│ • Sanitize environment variables                            │
│ • Validate Content-Length vs actual body size              │
│ • Limit POST body size                                      │
└─────────────────────────────────────────────────────────────┘
```

### Error Handling Scenarios:
```
CGI Error Handling:

┌─────────────────────────────────────────────────────────────┐
│ Error Type           │ Detection             │ Response      │
├─────────────────────────────────────────────────────────────┤
│ Script not found     │ access() returns -1   │ 404 Not Found │
│ Permission denied    │ execve() fails        │ 403 Forbidden │
│ Script timeout       │ waitpid() timeout     │ 504 Timeout   │
│ Script crash         │ WEXITSTATUS != 0      │ 500 Error     │
│ Pipe creation fail   │ pipe() returns -1     │ 500 Error     │
│ Fork failure         │ fork() returns -1     │ 500 Error     │
│ Invalid output       │ No headers/malformed  │ 502 Bad Gate  │
└─────────────────────────────────────────────────────────────┘

Error Recovery Flow:
┌─────────────────┐
│ CGI Error       │
│ Detected        │
└─────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Clean up        │ ──► │ • Close pipes                       │
│ resources       │     │ • Kill child process if needed      │
│                 │     │ • Free allocated memory             │
└─────────────────┘     └─────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────────────────────────┐
│ Generate error  │ ──► │ • Create appropriate HTTP error     │
│ response        │     │ • Log error details                 │
│                 │     │ • Return 500/502/504 response      │
└─────────────────┘     └─────────────────────────────────────┘
```

## 7. Performance and Optimization

### CGI Performance Characteristics:
```
Performance Analysis:

┌─────────────────────────────────────────────────────────────┐
│ Operation               │ Time Complexity    │ Notes         │
├─────────────────────────────────────────────────────────────┤
│ Environment setup       │ O(h)              │ h = headers   │
│ Process creation        │ O(1)              │ fork() cost   │
│ Script execution        │ O(script)         │ Script depend │
│ I/O communication       │ O(data_size)      │ Read/write    │
│ Response parsing        │ O(response_size)  │ Header parse  │
└─────────────────────────────────────────────────────────────┘

Memory Usage:
• Environment array: ~1KB per request
• Pipe buffers: ~8KB per request  
• Response buffer: Variable (script output size)
• Total overhead: ~10KB + response size

Optimization Opportunities:
• Reuse environment variable maps
• Implement script result caching
• Use FastCGI for persistent processes
• Optimize pipe buffer sizes
```

This comprehensive CGI analysis demonstrates how the web server executes external scripts safely and efficiently while maintaining proper process isolation and resource management.
