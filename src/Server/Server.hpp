#pragma once

#include <string>
#include "../HTTP/Request.hpp"

int create_server_socket(const std::string &host, int port);
void handle_requests(int server_fd, requestParser &req);
