#pragma once

#include <map>
#include <string>
#include <fcntl.h>
#include <Client.hpp>
#include <sys/epoll.h>
#include "../HTTP/Request.hpp"

class Server {
private:
	int epoll_fd;
	std::map<int, Client*> clients;
	static const int MAX_EVENTS = 64;
public:
	Server();
	~Server();

	void start(const std::string &host, int port);
	void stop();
	void handleConnections(int server_fd);
	void acceptNewConnection(int server_fd);
	void handleClientRead(int client_fd);
	void handleClientWrite(int client_fd);
	void handleClientRequest(int client_fd, requestParser &req);
	void sendResponse(int client_fd, const std::string &response);
	void closeClientConnection(int client_fd);
};

int create_server_socket(const std::string &host, int port);
void handle_requests(int server_fd, requestParser &req);
