#pragma once

#include <string>
#include <vector>
#include <map>
#include <sys/select.h>
#include "../HTTP/Request.hpp"
#include "../HTTP/Response.hpp"
#include "Client.hpp"

class Server {
private:
	int _server_fd;
	std::string _host;
	int _port;
	fd_set _master_read_fds;  // Master file descriptor list for reading
	fd_set _master_write_fds; // Master file descriptor list for writing
	fd_set _read_fds;         // Temp file descriptor list for select()
	fd_set _write_fds;        // Temp file descriptor list for select()
	int _max_fd;              // Maximum file descriptor number
	std::map<int, Client> _clients; // Map of client connections

	public:
	Server(const std::string &host, int port);
	~Server();

	bool setup();
	void run();
	void handleNewConnection();
	void handleClientData(int client_fd);
	void sendResponse(int client_fd);
	void removeClient(int client_fd);

	// Getter methods for host and port
	const std::string& getHost() const { return _host; }
	int getPort() const { return _port; }
};

// Keep these for backward compatibility
int create_server_socket(const std::string &host, int port);
void handle_requests(int server_fd, requestParser &req);
