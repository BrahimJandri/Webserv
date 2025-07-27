# HTTP Methods Implementation Analysis

## 1. GET Method Implementation

### Purpose and Flow
GET is used to retrieve resources from the server without modifying server state.

```
GET Method Processing Flow:

Client Request                 Server Processing               Response
      │                             │                            │
      ▼                             ▼                            ▼
┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
│ GET /index.html │ ──►  │ buildGetResponse│ ──►  │ 200 OK          │
│ Host: localhost │      │ ()              │      │ Content-Type:   │
│                 │      │                 │      │ text/html       │
└─────────────────┘      └─────────────────┘      │ <html>...</html>│
                                │                  └─────────────────┘
                                ▼
                    ┌─────────────────┐
                    │ Path Resolution │
                    │ & File Access   │
                    └─────────────────┘
```

### Detailed GET Implementation Analysis:

```cpp
Response Response::buildGetResponse(const requestParser &request, 
                                   const std::string &docRoot, 
                                   const bool autoindex, 
                                   int client_fd, 
                                   const ConfigParser::ServerConfig &serverConfig)
```

#### Step 1: Path Validation
```
Input Validation:
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Check docRoot   │ ──►│ docRoot empty?  │ ──►│ Return 404      │
│ parameter       │    │ YES             │    │ error           │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │ NO
         │                       ▼
         │              ┌─────────────────┐
         └─────────────►│ Continue        │
                        │ processing      │
                        └─────────────────┘
```

#### Step 2: File Existence Check
```cpp
if (!fileExists(fullPath)) {
    send_error_response(client_fd, 404, "Not Found", serverConfig);
    return Response();
}
```

#### Step 3: Directory Handling Logic
```
Directory Processing Decision Tree:

Is target a directory?
         │
    ┌────┴────┐
   YES        NO
    │          │
    ▼          ▼
┌─────────┐ ┌─────────┐
│Add '/'  │ │File     │
│to path  │ │Access   │
└─────────┘ │Check    │
    │       └─────────┘
    ▼
Has index file configured?
         │
    ┌────┴────┐
   YES        NO
    │          │
    ▼          ▼
┌─────────┐ ┌─────────┐
│Index    │ │Autoindex│
│file     │ │enabled? │
│exists?  │ └─────────┘
└─────────┘     │
    │      ┌────┴────┐
┌───┴───┐ YES        NO
│YES    │  │          │
│       │  ▼          ▼
│   ┌───▼──────┐ ┌─────────┐
│   │Generate  │ │403      │
│   │autoindex │ │Forbidden│
│   │page      │ └─────────┘
│   └──────────┘
│
▼
Serve index file
```

#### Step 4: File Access Permission Check
```cpp
if (access(fullPath.c_str(), R_OK) != 0) {
    send_error_response(client_fd, 403, "Forbidden", serverConfig);
    return Response();
}
```

### GET Method State Diagram:
```
GET Request Lifecycle:

┌─────────────────┐
│ Request         │
│ Received        │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Validate        │ ───────► 404 Not Found
│ Document Root   │ (if empty)
└─────────────────┘
         │ (valid)
         ▼
┌─────────────────┐
│ Check File      │ ───────► 404 Not Found
│ Existence       │ (if missing)
└─────────────────┘
         │ (exists)
         ▼
┌─────────────────┐
│ Is Directory?   │
└─────────────────┘
         │
    ┌────┴────┐
   YES        NO
    │          │
    ▼          ▼
┌─────────┐ ┌─────────────────┐
│Handle   │ │ Check Read      │ ──► 403 Forbidden
│Directory│ │ Permissions     │     (if denied)
│Logic    │ └─────────────────┘
└─────────┘          │ (allowed)
    │                ▼
    │       ┌─────────────────┐
    │       │ Build File      │
    │       │ Response        │
    │       └─────────────────┘
    │                │
    ▼                ▼
┌─────────────────────────────┐
│ Return Response Object      │
└─────────────────────────────┘
```

## 2. POST Method Implementation

### Purpose and Complexity
POST is used to submit data to the server, often causing state changes or side effects.

### POST Method Architecture:
```
POST Request Processing Architecture:

┌─────────────────────────────────────────────────────────────┐
│                    POST Method Handler                      │
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐               │
│  │ Content-Type    │    │ Data Processing │               │
│  │ Detection       │ ──►│ Pipeline        │               │
│  └─────────────────┘    └─────────────────┘               │
│           │                       │                       │
│           ▼                       ▼                       │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              Processing Branches                    │  │
│  │                                                     │  │
│  │ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐   │  │
│  │ │Form URL     │ │Multipart    │ │JSON Data    │   │  │
│  │ │Encoded      │ │Form Data    │ │Processing   │   │  │
│  │ │Processor    │ │(File Upload)│ │             │   │  │
│  │ └─────────────┘ └─────────────┘ └─────────────┘   │  │
│  └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Content-Type Branching Logic:
```
POST Content-Type Decision Tree:

Content-Type Header Analysis
            │
            ▼
┌─────────────────────────────────────────────────────┐
│ Extract Content-Type from headers                   │
│ (case-insensitive search)                          │
└─────────────────────────────────────────────────────┘
            │
            ▼
┌─────────────────────────────────────────────────────┐
│ Content-Type matching:                              │
├─────────────────────────────────────────────────────┤
│ Contains "application/x-www-form-urlencoded"?       │
│          │                                          │
│      ┌───┴───┐                                     │
│     YES      NO                                     │
│      │        │                                     │
│      ▼        ▼                                     │
│ ┌─────────┐ ┌─────────────────────────────────────┐ │
│ │Form     │ │ Contains "multipart/form-data"?     │ │
│ │Data     │ │          │                          │ │
│ │Handler  │ │      ┌───┴───┐                     │ │
│ └─────────┘ │     YES      NO                     │ │
│             │      │        │                     │ │
│             │      ▼        ▼                     │ │
│             │ ┌─────────┐ ┌─────────────────────┐ │ │
│             │ │File     │ │Contains             │ │ │
│             │ │Upload   │ │"application/json"?  │ │ │
│             │ │Handler  │ │       │             │ │ │
│             │ └─────────┘ │   ┌───┴───┐         │ │ │
│             │             │  YES      NO         │ │ │
│             │             │   │        │         │ │ │
│             │             │   ▼        ▼         │ │ │
│             │             │┌─────────┐┌─────────┐│ │ │
│             │             ││JSON     ││Default  ││ │ │
│             │             ││Handler  ││Text     ││ │ │
│             │             │└─────────┘│Handler  ││ │ │
│             │             │           └─────────┘│ │ │
│             └─────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

### Form URL-Encoded Processing:
```
application/x-www-form-urlencoded Processing:

Raw POST Body: "name=John+Doe&email=john%40example.com&age=25"
         │
         ▼
┌─────────────────┐
│ Split by '&'    │ ──► ["name=John+Doe", "email=john%40example.com", "age=25"]
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ For each pair:  │
│ Split by '='    │ ──► [("name", "John+Doe"), ("email", "john%40example.com"), ("age", "25")]
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ URL Decode      │ ──► [("name", "John Doe"), ("email", "john@example.com"), ("age", "25")]
│ values          │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Store in        │ ──► std::map<std::string, std::string> formFields
│ map structure   │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Save to file    │ ──► docRoot/data/formData.txt
│ and create      │
│ HTML response   │
└─────────────────┘
```

### Multipart Form Data Processing:
```
multipart/form-data Processing Flow:

Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW
         │
         ▼
┌─────────────────┐
│ Extract         │ ──► boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW"
│ Boundary        │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Split body by   │ ──► [part1, part2, part3, ...]
│ boundary        │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ For each part:  │
│                 │
│ 1. Extract      │ ──► Content-Disposition: form-data; name="file"; filename="test.txt"
│    headers      │     Content-Type: text/plain
│                 │
│ 2. Parse name   │ ──► name = "file", filename = "test.txt"
│    and filename │
│                 │
│ 3. Extract      │ ──► "Hello, this is file content!"
│    content      │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ If filename     │ ──► Save to: docRoot/filename
│ exists: Save    │
│ file            │
│                 │
│ Else: Store     │ ──► formFields[name] = content
│ form field      │
└─────────────────┘
```

### File Upload Memory Management:
```
File Upload Processing Memory Flow:

HTTP Request Buffer:
┌─────────────────────────────────────────────────────────────┐
│ POST /upload HTTP/1.1\r\n                                  │
│ Content-Type: multipart/form-data; boundary=xyz\r\n        │
│ Content-Length: 1048576\r\n                                │
│ \r\n                                                       │
│ --xyz\r\n                                                  │
│ Content-Disposition: form-data; name="file"; filename=...   │
│ [1MB of file data]                                         │
│ --xyz--\r\n                                                │
└─────────────────────────────────────────────────────────────┘
         │ (parsed in memory)
         ▼
Temporary Content String:
┌─────────────────────────────────────────────────────────────┐
│ std::string content = part.substr(headerEnd + 4);          │
│ [1MB file content in memory]                               │
└─────────────────────────────────────────────────────────────┘
         │ (written to disk)
         ▼
File System:
┌─────────────────────────────────────────────────────────────┐
│ docRoot/uploaded_file.jpg                                   │
│ [1MB file saved to disk]                                   │
└─────────────────────────────────────────────────────────────┘
         │ (content string destroyed)
         ▼
Memory Released:
┌─────────────────────────────────────────────────────────────┐
│ Temporary content string deallocated                       │
│ Only small response HTML remains in memory                 │
└─────────────────────────────────────────────────────────────┘
```

## 3. DELETE Method Implementation

### Purpose and Security
DELETE is used to remove resources from the server with strict security controls.

### DELETE Method Security Architecture:
```
DELETE Security Layers:

┌─────────────────────────────────────────────────────────────┐
│                    Security Validation Pipeline             │
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐               │
│  │ Path            │    │ File System     │               │
│  │ Validation      │ ──►│ Permissions     │               │
│  └─────────────────┘    └─────────────────┘               │
│           │                       │                       │
│           ▼                       ▼                       │
│  ┌─────────────────┐    ┌─────────────────┐               │
│  │ Dangerous Path  │    │ Write Access    │               │
│  │ Detection       │    │ Check           │               │
│  └─────────────────┘    └─────────────────┘               │
│           │                       │                       │
│           ▼                       ▼                       │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              Deletion Execution                     │  │
│  │                                                     │  │
│  │ ┌─────────────┐          ┌─────────────┐           │  │
│  │ │Directory    │          │File         │           │  │
│  │ │Recursive    │          │Deletion     │           │  │
│  │ │Deletion     │          │             │           │  │
│  │ └─────────────┘          └─────────────┘           │  │
│  └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Path Validation Security:
```
Dangerous Path Detection:

Input Path Validation:
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ requestPath     │ ──►│ Security Check  │ ──►│ Action          │
├─────────────────┤    ├─────────────────┤    ├─────────────────┤
│ ""              │    │ Empty path      │    │ 400 Bad Request │
│ "/"             │    │ Root path       │    │ 400 Bad Request │
│ "../etc/passwd" │    │ Contains ".."   │    │ 400 Bad Request │
│ "/safe/file.txt"│    │ Valid path      │    │ Continue        │
└─────────────────┘    └─────────────────┘    └─────────────────┘

Security Rules:
• Reject empty paths
• Reject root path deletions  
• Reject paths containing ".." (directory traversal)
• Validate write permissions
• Check file/directory existence
```

### Directory vs File Deletion:
```
Deletion Type Determination:

stat(fullPath) system call
         │
         ▼
┌─────────────────┐
│ Check file type │
│ using st_mode   │
└─────────────────┘
         │
    ┌────┴────┐
   │         │
   ▼         ▼
S_ISDIR()   S_ISREG()
   │         │
   ▼         ▼
┌─────────┐ ┌─────────┐
│Directory│ │Regular  │
│Deletion │ │File     │
│         │ │Deletion │
└─────────┘ └─────────┘
   │         │
   ▼         ▼
┌─────────┐ ┌─────────┐
│Recursive│ │remove() │
│deleteDir│ │system   │
│()       │ │call     │
└─────────┘ └─────────┘
```

### Recursive Directory Deletion Algorithm:
```cpp
bool deleteDirectory(const std::string &path) {
    // Open directory
    DIR *dir = opendir(path.c_str());
    
    // Read each entry
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (name == "." || name == "..") continue;
        
        // Build full path
        std::string fullPath = path + "/" + name;
        
        // Check if it's a directory
        if (S_ISDIR(entryStat.st_mode)) {
            // Recursive call
            deleteDirectory(fullPath);
        } else {
            // Delete file
            remove(fullPath.c_str());
        }
    }
    
    // Delete the now-empty directory
    rmdir(path.c_str());
}
```

Recursive Deletion Tree Traversal:
```
Directory Structure:          Deletion Order:
/target/                      
├── file1.txt                 1. Delete file1.txt
├── file2.txt                 2. Delete file2.txt  
├── subdir1/                  3. Enter subdir1/
│   ├── file3.txt             4. Delete file3.txt
│   └── file4.txt             5. Delete file4.txt
│                             6. Delete subdir1/ (now empty)
├── subdir2/                  7. Enter subdir2/
│   ├── file5.txt             8. Delete file5.txt
│   └── nested/               9. Enter nested/
│       └── file6.txt         10. Delete file6.txt
│                             11. Delete nested/ (now empty)
│                             12. Delete subdir2/ (now empty)
└── file7.txt                 13. Delete file7.txt
                              14. Delete target/ (now empty)
```

## 4. HTTP Status Code Management

### Status Code Categories:
```
HTTP Status Code Hierarchy:

┌─────────────────────────────────────────────────────────────┐
│                    HTTP Status Codes                       │
│                                                             │
│ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│ │2xx Success  │ │4xx Client   │ │5xx Server   │           │
│ │             │ │Error        │ │Error        │           │
│ │• 200 OK     │ │• 400 Bad    │ │• 500 Internal│          │
│ │• 201 Created│ │  Request    │ │  Server Error│          │
│ │             │ │• 403 Forbidden│ │             │          │
│ │             │ │• 404 Not Found│ │             │          │
│ │             │ │• 405 Method │ │             │          │
│ │             │ │  Not Allowed│ │             │          │
│ │             │ │• 413 Payload│ │             │          │
│ │             │ │  Too Large  │ │             │          │
│ └─────────────┘ └─────────────┘ └─────────────┘           │
└─────────────────────────────────────────────────────────────┘
```

### Error Response Generation:
```
Error Response Flow:

Error Condition Detected
         │
         ▼
┌─────────────────┐
│ Determine       │ ──► Status code mapping
│ Status Code     │     400, 403, 404, 405, 413, 500
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Load Custom     │ ──► Check serverConfig.error_pages
│ Error Page      │     map for custom error pages
└─────────────────┘
         │
    ┌────┴────┐
   FOUND      NOT FOUND
    │          │
    ▼          ▼
┌─────────┐ ┌─────────────────┐
│Use      │ │Use Default      │ ──► ./www/epages/404.html
│Custom   │ │Error Page       │     ./www/epages/500.html
│Page     │ │                 │     etc.
└─────────┘ └─────────────────┘
    │          │
    └────┬─────┘
         ▼
┌─────────────────┐
│ Generate HTML   │ ──► Complete HTTP response with
│ Response        │     status line, headers, and body
└─────────────────┘
```

This comprehensive analysis shows how the Response class and HTTP methods work together to handle different types of client requests, from simple file serving to complex file uploads and secure deletions.
