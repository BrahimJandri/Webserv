#include "Server.hpp"
#include "../Config/ConfigParser.hpp"
#include "../Utils/Logger.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// Original functions kept for backward compatibility
int create_server_socket(const std::string &host, int port)
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		perror("socket");
		return -1;
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);                    
	addr.sin_addr.s_addr = inet_addr(host.c_str()); 

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("bind");
		close(server_fd);
		return -1;
	}

	if (listen(server_fd, 10) == -1)
	{
		perror("listen");
		close(server_fd);
		return -1;
	}

	std::cout << "Listening on " << host << ":" << port << std::endl;
	return server_fd;
}

std::string to_string_c98(size_t val)
{
	std::ostringstream oss;
	oss << val;
	return oss.str();
}

void handle_requests(int server_fd, requestParser &req)
{
	while (true)
	{
		int client_fd = accept(server_fd, NULL, NULL);
		if (client_fd < 0)
		{
			perror("accept");
			continue;
		}

		char buffer[4096];
		std::memset(buffer, 0, sizeof(buffer));
		ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
		if (bytes_read <= 0)
		{
			close(client_fd);
			continue;
		}

		std::string raw_request(buffer);
		req.parseRequest(raw_request);

		std::cout << "== New HTTP Request ==\n";
		std::cout << req.getMethod() << " " << req.getPath() << " " << req.getHttpVersion() << std::endl;
		std::map<std::string, std::string>::const_iterator it;
		for (it = req.getHeaders().begin(); it != req.getHeaders().end(); ++it)
		{
			std::cout << it->first << ": " << it->second << std::endl;
		}

		std::string body = "<h1>Hello from Webserv</h1>";
		std::string response =
			"HTTP/ 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: " +
			to_string_c98(body.length()) + "\r\n"
											"\r\n" +
			body;
		write(client_fd, response.c_str(), response.length());
		close(client_fd);
	}
}

// New Server class implementation with multiplexing
Server::Server(const std::string &host, int port) 
    : _server_fd(-1), _host(host), _port(port), _max_fd(0) {
    FD_ZERO(&_master_read_fds);
    FD_ZERO(&_master_write_fds);
}

Server::~Server() {
    // Close all client connections
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
    }
    
    // Close server socket
    if (_server_fd != -1) {
        close(_server_fd);
    }
}

bool Server::setup() {
    // Create socket
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1) {
        perror("socket");
        return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(_server_fd);
        return false;
    }

    // Set non-blocking mode
    int flags = fcntl(_server_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl (F_GETFL)");
        close(_server_fd);
        return false;
    }
    
    if (fcntl(_server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl (F_SETFL)");
        close(_server_fd);
        return false;
    }

    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = inet_addr(_host.c_str());

    if (bind(_server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(_server_fd);
        return false;
    }

    // Listen
    if (listen(_server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(_server_fd);
        return false;
    }

    // Add server socket to master set
    FD_SET(_server_fd, &_master_read_fds);
    _max_fd = _server_fd;

    std::cout << "Server listening on " << _host << ":" << _port << std::endl;
    return true;
}

void Server::run() {
    while (true) {
        // Copy master sets to working sets
        _read_fds = _master_read_fds;
        _write_fds = _master_write_fds;

        // Wait for activity
        int activity = select(_max_fd + 1, &_read_fds, &_write_fds, NULL, NULL);
        if (activity < 0) {
            perror("select");
            break;
        }

        // Check for activity on server socket (new connection)
        if (FD_ISSET(_server_fd, &_read_fds)) {
            handleNewConnection();
        }

        // Check for activity on client sockets
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ) {
            int client_fd = it->first;
            
            // Handle data from client
            if (FD_ISSET(client_fd, &_read_fds)) {
                handleClientData(client_fd);
            }
            
            // Send response to client if ready
            if (FD_ISSET(client_fd, &_write_fds)) {
                sendResponse(client_fd);
            }

            // Move to next client before potential removal
            ++it;
        }
    }
}

void Server::handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(_server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }
    
    // Set non-blocking mode for client socket
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl (F_GETFL)");
        close(client_fd);
        return;
    }
    
    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl (F_SETFL)");
        close(client_fd);
        return;
    }
    
    // Add to clients map
    _clients[client_fd] = Client(client_fd);
    
    // Add to master read set
    FD_SET(client_fd, &_master_read_fds);
    
    // Update max_fd if necessary
    if (client_fd > _max_fd) {
        _max_fd = client_fd;
    }
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port)
              << " on socket " << client_fd << std::endl;
}

void Server::handleClientData(int client_fd) {
    char buffer[4096];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        // Error or connection closed by client
        if (bytes_read < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("read");
            }
        }
        
        removeClient(client_fd);
        return;
    }
    
    // Append data to client buffer
    buffer[bytes_read] = '\0';
    _clients[client_fd].appendToBuffer(std::string(buffer));
    
    // Process request
    if (_clients[client_fd].processRequest()) {
        // Request is complete, prepare response
        _clients[client_fd].prepareResponse();
        
        // Add to write set to send response
        FD_SET(client_fd, &_master_write_fds);
        
        // Log request information
        const requestParser &req = _clients[client_fd].getRequest();
        std::cout << "== New HTTP Request on socket " << client_fd << " ==\n";
        std::cout << req.getMethod() << " " << req.getPath() << " " << req.getHttpVersion() << std::endl;
        
        const std::map<std::string, std::string> &headers = req.getHeaders();
        for (std::map<std::string, std::string>::const_iterator it = headers.begin();
             it != headers.end(); ++it) {
            std::cout << it->first << ": " << it->second << std::endl;
        }
    }
}

void Server::sendResponse(int client_fd) {
    if (!_clients[client_fd].isResponseReady()) {
        return;
    }
    
    std::string response = _clients[client_fd].getResponse();
    ssize_t bytes_sent = write(client_fd, response.c_str(), response.length());
    
    if (bytes_sent <= 0) {
        if (bytes_sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("write");
        }
        removeClient(client_fd);
        return;
    }
    
    std::cout << "Response sent to client on socket " << client_fd << std::endl;
    
    // After sending response, we're done with this client for HTTP/1.0
    // For HTTP/1.1, we could keep the connection open for more requests
    // For now, we'll close after each request
    removeClient(client_fd);
}

void Server::removeClient(int client_fd) {
    // Remove from sets
    FD_CLR(client_fd, &_master_read_fds);
    FD_CLR(client_fd, &_master_write_fds);
    
    // Close socket
    close(client_fd);
    
    // Remove from clients map
    _clients.erase(client_fd);
    
    std::cout << "Connection closed on socket " << client_fd << std::endl;
    
    // Recalculate _max_fd if necessary
    if (client_fd == _max_fd) {
        _max_fd = _server_fd; // Start with server socket
        
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            if (it->first > _max_fd) {
                _max_fd = it->first;
            }
        }
    }
}
