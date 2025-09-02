# Webserv

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
```

### Testing the Server
Once running, you can test the server:

```bash
# Basic GET request
curl http://localhost:8080/

# Upload a file
curl -X POST -F "file=@example.txt" http://localhost:8080/upload/

# Access CGI script
curl http://localhost:8080/cgi-bin/script.py
```

## 📁 Project Structure

```
Webserv/
├── src/                    # Source code
│   ├── Server/            # Server management and connection handling
│   ├── Client/            # Client connection management
│   ├── Request/           # HTTP request parsing
│   ├── Response/          # HTTP response generation
│   ├── Parser/            # Configuration file parsing
│   ├── CGI/               # CGI script execution
│   ├── Utils/             # Utility functions and logging
│   └── main.cpp           # Application entry point
├── conf/                  # Configuration files
│   ├── default.conf       # Default server configuration
│   ├── good_confs/        # Example working configurations
│   └── broken_confs/      # Test configurations for error handling
├── www/                   # Web content directory
│   ├── index.html         # Default home page
│   ├── upload/            # File upload directory
│   ├── cgi-bin/           # CGI scripts
│   └── epages/            # Error page templates
├── Makefile               # Build configuration
├── README.md              # This file
└── ARCHITECTURE.md        # Detailed technical documentation
```

## ⚙️ Configuration Options

### Server Block Directives

| Directive | Description | Example |
|-----------|-------------|---------|
| `listen` | Server listening address and port | `listen 127.0.0.1:8080;` |
| `server_name` | Server name for virtual hosting | `server_name example.com;` |
| `root` | Document root directory | `root ./www;` |
| `limit_client_body_size` | Maximum request body size | `limit_client_body_size 10M;` |
| `error_page` | Custom error page mapping | `error_page 404 /404.html;` |

### Location Block Directives

| Directive | Description | Example |
|-----------|-------------|---------|
| `root` | Root directory for this location | `root ./www/static;` |
| `index` | Default file to serve | `index index.html index.htm;` |
| `allowed_methods` | Permitted HTTP methods | `allowed_methods GET POST DELETE;` |
| `autoindex` | Enable directory listing | `autoindex on;` |
| `return` | HTTP redirect | `return https://example.com;` |
| `cgi_map` | CGI script mapping | `cgi_map .py /usr/bin/python3;` |

## 🧪 Testing

The project includes various test configurations and web content:

```bash
# Test with different configurations
./Webserv conf/good_confs/basic.conf
./Webserv conf/good_confs/multiple_servers.conf

# Access test pages
curl http://localhost:8080/
curl http://localhost:8080/upload.html
curl http://localhost:8080/cgi.html
```

## 🔧 Development

### Building for Development
```bash
# Enable debug symbols (modify Makefile if needed)
make CXXFLAGS="-Wall -Wextra -Werror -std=c++98 -g"

# Check for memory leaks
valgrind --leak-check=full ./Webserv
```

### Code Style
- **Standard**: C++98 compliant
- **Compiler Flags**: `-Wall -Wextra -Werror`
- **Naming**: Consistent with project conventions
- **Documentation**: See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed technical documentation

## 📚 Technical Details

This implementation features:

- **Event-Driven Architecture**: Uses Linux epoll for efficient I/O multiplexing
- **State Machine**: Robust request/response state management
- **Memory Management**: Careful resource management without memory leaks
- **Error Handling**: Comprehensive error checking and graceful degradation
- **Performance**: Optimized for handling thousands of concurrent connections

For detailed technical documentation, see [ARCHITECTURE.md](ARCHITECTURE.md).

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines
- Follow C++98 standard
- Maintain existing code style
- Add tests for new features
- Update documentation as needed
- Ensure no memory leaks (test with valgrind)

## 📄 License

This project is part of the 42 School curriculum. Please respect academic integrity if you're a 42 student.

## 🙏 Acknowledgments

- **42 School** for the project specifications
- **nginx** for configuration syntax inspiration
- **Contributors** to the HTTP/1.1 specification

## 📞 Support

If you encounter any issues or have questions:

1. Check the [ARCHITECTURE.md](ARCHITECTURE.md) for technical details
2. Review the example configurations in `conf/good_confs/`
3. Test with the provided web content in `www/`
4. Open an issue on GitHub for bug reports or feature requests

---

*Built with ❤️ using C++98*