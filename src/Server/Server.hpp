#pragma once

#include <string>
#include "../HTTP/Request.hpp"
// #include "../CGI/CGIHandler.hpp" // Include the CGI handler for handling CGI requests
// #include "../HTTP/Response.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <map>
#include <fcntl.h>
#include <sys/epoll.h>
#include "../CGI/CookieManager.hpp"
#include "Client.hpp"

class Client; // Forward declaration to avoid circular dependency

class Server
{
private:
    int epoll_fd;
    std::map<int, Client *> clients;
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
    static void sendResponse(int client_fd, const std::string &response);
    void closeClientConnection(int client_fd);
};

int create_server_socket(const std::string &host, int port);
void handle_requests(int server_fd, requestParser &req);
