
![Webserv Logo](webserv.png)

A high-performance HTTP server implementation written in C++98, featuring event-driven architecture with epoll for efficient handling of multiple concurrent connections.

## 🚀 Features

- **HTTP/1.1 Support**: Full compliance with HTTP/1.1 protocol
- **Multiple Virtual Hosts**: Support for multiple server configurations on different ports
- **Static File Serving**: Efficient static content delivery
- **CGI Support**: Execute Python and shell scripts via CGI
- **File Upload/Download**: Handle file transfers with configurable size limits
- **Directory Listing**: Automatic directory indexing when enabled
- **Custom Error Pages**: Configurable error page templates
- **Request Methods**: Support for GET, POST, and DELETE methods
- **Configuration File**: Nginx-style configuration syntax
- **Event-Driven Architecture**: High-performance epoll-based event handling
- **Non-blocking I/O**: Asynchronous request processing
- **Connection Management**: Efficient client connection handling

## 📋 Requirements

- **Compiler**: g++ or clang++ with C++98 standard support
- **Operating System**: Linux (epoll required)
- **Build System**: GNU Make

## 🛠️ Installation

### 1. Clone the Repository
```bash
git clone https://github.com/BrahimJandri/Webserv.git
cd Webserv
```

### 2. Build the Project
```bash
make
```

### 3. Clean Build (if needed)
```bash
make fclean  # Remove all build artifacts
make re      # Clean and rebuild
```

## 🎯 Usage

### Basic Usage
```bash
# Run with default configuration
./Webserv

# Run with custom configuration file
./Webserv path/to/config.conf
```

### Example Configuration
The server uses nginx-style configuration files. Here's a basic example:

```nginx
server {
    listen 127.0.0.1:8080;
    server_name mywebsite.com;
    root ./www;
    limit_client_body_size 100M;
    
    # Error pages
    error_page 404 /epages/404.html;
    error_page 500 /epages/500.html;
    
    # Static files location
    location / {
        root ./www;
        index index.html;
        allowed_methods GET POST;
        autoindex off;
    }
    
    # File upload location
    location /upload {
        root ./www;
        allowed_methods GET POST DELETE;
        autoindex on;
    }
    
    # CGI scripts
    location /cgi-bin {
        cgi_map .py /usr/bin/python3;
        cgi_map .sh /bin/sh;
    }
}
