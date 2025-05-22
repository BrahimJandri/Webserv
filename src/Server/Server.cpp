#include "Server.hpp"
#include "../Config/ConfigParser.hpp"
#include "../Utils/Logger.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Server::Server()
{
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
	{
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}
}

Server::~Server()
{
	close(epoll_fd);
	for (std::map<int, Client *>::iterator iter = clients.begin(); iter != clients.end(); ++iter)
	{
		delete iter->second;
	}
}

void Server::start(const std::string &host, int port)
{
	int server_fd = create_server_socket(host, port);
	if (server_fd == -1)
	{
		std::cerr << "Failed to create server socket." << std::endl;
		exit(EXIT_FAILURE);
	}

	requestParser req;
	handle_requests(server_fd, req);

	close(server_fd);
}

void Server::stop()
{
	for (std::map<int, Client *>::iterator iter = clients.begin(); iter != clients.end(); ++iter)
	{
		close(iter->first);
		delete iter->second;
	}
	clients.clear();
}

void Server::acceptNewConnection(int server_fd)
{
	struct sockaddr_in	client_addr;
	socklen_t	client_len = sizeof(client_addr);

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd == -1)
	{
		perror("accept");
		return ;
	}

	// Set non-blocking
	int flags = fcntl(client_fd, F_GETFL, 0);
	fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

	// Add client to epoll
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET; // Edge triggered
	event.data.fd = client_fd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
	{
		perror("epoll_ctl: client_fd");
		close(client_fd);
		return ;
	}

	// Create client object and store it
	clients[client_fd] = new Client(client_fd);

	std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr)
				<< ":" << ntohs(client_addr.sin_port)
				<< " (fd: " << client_fd << ")" << std::endl;
}

void Server::handleClientRead(int client_fd)
{
	if (clients.find(client_fd) == clients.end())
	{
		// Client not found, something is wrong
		closeClientConnection(client_fd);
		return ;
	}

	char buffer[4096];
	ssize_t bytes_read;

	//Read data in a loop until EAGAIN/EWOULDBLOCK
	while (true)
	{
		bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
		
		if (bytes_read == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				// No more data to read
				break;
			}
			else
			{
				// Error occurred
				perror("read");
				closeClientConnection(client_fd);
				return;
			}
		}
		else if (bytes_read == 0)
		{
			// Client closed connection
			closeClientConnection(client_fd);
			return;
		}
		else
		{
			// Data received, append to client's buffer
			buffer[bytes_read] = '\0';
			clients[client_fd]->appendToBuffer(std::string(buffer, bytes_read));
		}
	}

	// Process the complete request if we have one
	if (clients[client_fd]->processRequest())
	{
		// Request is complete, prepare response
		clients[client_fd]->prepareResponse();
		
		// Update epoll to watch for write readiness
		struct epoll_event event;
		event.events = EPOLLOUT | EPOLLET;
		event.data.fd = client_fd;
		
		if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1)
		{
			perror("epoll_ctl: modify to EPOLLOUT");
			closeClientConnection(client_fd);
		}
	}
}

void Server::handleClientWrite(int client_fd)
{
	if (clients.find(client_fd) == clients.end())
	{
		closeClientConnection(client_fd);
		return;
	}

	Client* client = clients[client_fd];

	if (!client->isResponseReady())
	{
		// Should not happen, but just in case
		client->prepareResponse();
	}

	std::string response = client->getResponse();
	ssize_t bytes_sent = write(client_fd, response.c_str(), response.length());

	if (bytes_sent == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("write");
			closeClientConnection(client_fd);
		}
		return;
	}

	// For simplicity, we'll close the connection after sending the response
	// In a real HTTP server, you might want to check for Keep-Alive header
	closeClientConnection(client_fd);
}

void Server::handleConnections(int server_fd)
{
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = server_fd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
	{
		perror("Epoll_ctl: server_fd!");
		exit(EXIT_FAILURE);
	}
	struct epoll_event events[MAX_EVENTS];

	while (true)
	{
		int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (num_events == -1)
		{
			perror("epoll_wait");
			continue;
		}

		for (int i = 0; i < num_events; i++) {
			if (events[i].data.fd == server_fd)
				acceptNewConnection(server_fd);
			else if (events[i].events & EPOLLIN)
				handleClientRead(events[i].data.fd);
			else if (events[i].events & EPOLLOUT)
				handleClientWrite(events[i].data.fd);
		}
	}
}

void Server::handleClientRequest(int client_fd, requestParser &req)
{
	char buffer[4096];
	std::memset(buffer, 0, sizeof(buffer));
	ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
	if (bytes_read <= 0)
	{
		closeClientConnection(client_fd);
		return;
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
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: " + to_string_c98(body.length()) + "\r\n"
														   "\r\n" +
		body;
	sendResponse(client_fd, response);
}

void Server::sendResponse(int client_fd, const std::string &response)
{
	ssize_t bytes_written = write(client_fd, response.c_str(), response.length());
	if (bytes_written < 0)
	{
		perror("write");
	}
	closeClientConnection(client_fd);
}

void Server::closeClientConnection(int client_fd)
{
	if (clients.find(client_fd) != clients.end())
	{
		delete clients[client_fd];
		clients.erase(client_fd);
	}
	close(client_fd);
	std::cout << "Closed connection for client fd: " << client_fd << std::endl;
}

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
	addr.sin_port = htons(port);                    // This function is used to convert the unsigned int from machine byte order to network byte order.
	addr.sin_addr.s_addr = inet_addr(host.c_str()); // It is used when we don't want to bind our socket to any particular IP and instead make it listen to all the available IPs.

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
