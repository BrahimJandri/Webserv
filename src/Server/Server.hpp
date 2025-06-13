#pragma once

#include <string>
#include "../HTTP/Request.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int create_server_socket(const std::string &host, int port);
void handle_requests(int server_fd, requestParser &req);
