#include "../HTTP/ResponseBuilder.hpp"
#include "../Config/ConfigParser.hpp"
#include "../Utils/Logger.hpp" // Make sure Logger.hpp defines LOG_ERROR, LOG_INFO etc. if used
#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>	  // For open
#include <sys/stat.h> // For stat, fstat

// Helper function to convert size_t to std::string (C++98 compatible)
std::string to_string_c98(size_t val)
{
	std::ostringstream oss;
	oss << val;
	return oss.str();
}

Server::Server()
{
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
	{
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}
}
// Your create_server_socket and to_string_c98 functions remain the same
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

// Helper function to send an HTTP response (status line and headers only)
void send_http_headers(int client_fd, const std::string &status_line,
					   const std::string &content_type, size_t content_length,
					   const std::string &connection_header = "close")
{
	std::ostringstream oss;
	oss << status_line << "\r\n"
		<< "Content-Type: " << content_type << "\r\n"
		<< "Content-Length: " << content_length << "\r\n"
		<< "Connection: " << connection_header << "\r\n\r\n"; // End of headers

	std::string header_response = oss.str();
	write(client_fd, header_response.c_str(), header_response.length());
}

// Helper function to send an error response
void send_error_response(int client_fd, int status_code, const std::string &message)
{
	std::string status_text;
	std::string error_page_path;
	if (status_code == 404)
	{
		status_text = "Not Found";
		error_page_path = "Error_pages/404.html";
	}
	else if (status_code == 500)
	{
		status_text = "Internal Server Error";
		error_page_path = "Error_pages/500.html";
	}
	else
	{
		status_text = "Unknown Error";
		error_page_path = "Error_pages/500.html"; // Fallback
	}

	std::string response_body = message; // Default body if error page not found

	int error_fd = open(error_page_path.c_str(), O_RDONLY);
	if (error_fd >= 0)
	{
		struct stat st;
		fstat(error_fd, &st);
		response_body.resize(st.st_size);
		read(error_fd, &response_body[0], st.st_size);
		close(error_fd);
	}
	else
	{
		// If error page itself not found, use a simple HTML message
		response_body = "<!DOCTYPE html><html><head><title>Error " + to_string_c98(status_code) + "</title></head><body><h1>Error " + to_string_c98(status_code) + ": " + status_text + "</h1><p>" + message + "</p></body></html>";
	}

	send_http_headers(client_fd, "HTTP/1.1 " + to_string_c98(status_code) + " " + status_text,
					  "text/html", response_body.length());
	write(client_fd, response_body.c_str(), response_body.length());
}

// Helper function to serve a static file
void serve_static_file(int client_fd, const std::string &file_path, const std::string &content_type)
{
	int file_fd = open(file_path.c_str(), O_RDONLY);
	if (file_fd < 0)
	{
		// File not found or inaccessible, send 404
		perror(("open " + file_path).c_str());
		send_error_response(client_fd, 404, "The requested resource '" + file_path + "' was not found.");
		return;
	}

	struct stat st;
	fstat(file_fd, &st);
	size_t file_size = st.st_size;

	send_http_headers(client_fd, "HTTP/1.1 200 OK", content_type, file_size);

	char buffer[4096];
	ssize_t bytes_read;
	while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0)
	{
		write(client_fd, buffer, bytes_read);
	}

	close(file_fd);
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

// Your main request handling loop
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
		std::cout << "Method: " << req.getMethod() << "\n";
		std::cout << "Path: " << req.getPath() << "\n";
		std::cout << "HTTP Version: " << req.getHttpVersion() << std::endl;
		std::cout << "Headers:\n";
		for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin(); it != req.getHeaders().end(); ++it)
		{
			std::cout << "  " << it->first << ": " << it->second << std::endl;
		}
		std::cout << "Body:\n"
				  << req.getBody() << std::endl;

		std::string docRoot = "/home/bjandri/Desktop/webserb/www";
		std::string method = req.getMethod();
		std::string path = req.getPath();

		Response response;

		// === ✅ Handle CGI if path contains "/cgi-bin/"
		if ((method == "GET" || method == "POST") && path.find("/cgi-bin/") != std::string::npos)
		{
			std::cout << "Handling CGI request for path: " << std::endl;
			response.handleCGI(req);
		}
		else if (method == "GET")
		{
			response = ResponseBuilder::buildGetResponse(req, docRoot);
		}
		else if (method == "POST")
		{
			response = ResponseBuilder::buildPostResponse(req, docRoot);
		}
		else if (method == "DELETE")
		{
			response = ResponseBuilder::buildDeleteResponse(req, docRoot);
		}
		else
		{
			response = ResponseBuilder::buildErrorResponse(405, "Method Not Allowed");
		}

		Server::sendResponse(client_fd, response.toString());
		close(client_fd); // Don't forget to close the client socket
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
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd == -1)
	{
		perror("accept");
		return;
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
		return;
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
		return;
	}

	char buffer[4096];
	ssize_t bytes_read;

	// Read data in a loop until EAGAIN/EWOULDBLOCK
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

	Client *client = clients[client_fd];

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

		for (int i = 0; i < num_events; i++)
		{
			if (events[i].data.fd == server_fd)
				acceptNewConnection(server_fd);
			else if (events[i].events & EPOLLIN)
				handleClientRead(events[i].data.fd);
			else if (events[i].events & EPOLLOUT)
				handleClientWrite(events[i].data.fd);
		}
	}
}

void Server::sendResponse(int client_fd, const std::string &response)
{
	// Log the response being sent
	std::cout << "\n== HTTP Response ==\n";
	std::cout << response << std::endl;
	std::cout << "==================\n"
			  << std::endl;

	ssize_t bytes_written = write(client_fd, response.c_str(), response.length());
	if (bytes_written < 0)
		perror("write");
	// For the simple synchronous case, just close the fd directly
	// without epoll cleanup since this function is used in handle_requests
	// which doesn't use epoll
	close(client_fd);
}

void Server::closeClientConnection(int client_fd)
{
	// Remove from epoll first to avoid getting events for a closed fd
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1)
	{
		// Don't treat this as fatal - the fd might already be removed
		// perror("epoll_ctl: EPOLL_CTL_DEL");
	}

	// Clean up client object if it exists
	if (clients.find(client_fd) != clients.end())
	{
		delete clients[client_fd];
		clients.erase(client_fd);
	}
	// Close the file descriptor
	if (close(client_fd) == -1)
		perror("close");
	std::cout << "Closed connection for client fd: " << client_fd << std::endl;
}
