# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

- **Build**: `make` - Compiles the Webserv HTTP server executable
- **Clean**: `make clean` - Removes object files
- **Full clean**: `make fclean` - Removes object files and executable
- **Rebuild**: `make re` - Full clean and rebuild

## Running the Server

```bash
./Webserv [config_file]
```

- Default config: `./Webserv` (uses `./conf/default.conf`)
- Custom config: `./Webserv path/to/config.conf`

## Architecture Overview

This is a C++98 HTTP web server implementation with the following core components:

### Core Architecture
- **Single-threaded epoll-based event loop** (`src/Server/Server.cpp:20`) - The server uses epoll for efficient I/O multiplexing
- **Multi-server support** - Can listen on multiple ports/hosts from a single configuration file
- **Client connection management** - Each client connection is managed through the `Client` class

### Module Structure
- **Config** (`src/Config/`) - Configuration file parser supporting nginx-like syntax
- **Server** (`src/Server/`) - Core server logic with `Server` class managing connections and `Client` class handling individual client state
- **HTTP** (`src/HTTP/`) - HTTP request/response parsing and building
- **CGI** (`src/CGI/`) - CGI script execution and cookie management
- **Utils** (`src/Utils/`) - Logging utilities and ANSI color support

### Configuration System
- Uses nginx-like configuration syntax
- Supports multiple server blocks with different listen addresses
- Location-based routing with method restrictions
- CGI support with configurable extensions and paths
- Static file serving with autoindex support
- Custom error pages and redirects

### Key Classes
- `Server`: Main server class managing epoll loop and client connections
- `Client`: Represents individual client connections with request/response state
- `ConfigParser`: Parses configuration files into structured data
- `requestParser`/`Request`: HTTP request parsing and validation
- `Response`/`ResponseBuilder`: HTTP response generation
- `CGIHandler`: Executes CGI scripts and handles their output

### Development Notes
- Compiled with C++98 standard (`-std=c++98`)
- Uses strict compiler flags: `-Wall -Wextra -Werror`
- No external dependencies beyond standard C++ library and POSIX system calls
- Object files are organized in `objFiles/` directory