#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include "requestParser.hpp"

struct Location
{
    std::string path;
    std::string root;
    std::string index;
};


struct ServerConfig
{
    std::string host;
    int port;
    std::string server_name;
    std::vector<Location> locations;
    std::map<int, std::string> error_pages;
};

class ConfigParser
{
public:
    ConfigParser(const std::string &file_path);
    const std::vector<ServerConfig> &getServers() const;
    bool parse();

private:
    std::string _file_path;
    std::vector<ServerConfig> _servers;

    void _parseServerBlock(std::ifstream &file);
    void _trim(std::string &line);
};

int create_server_socket(const std::string &host, int port);
void handle_requests(int server_fd, requestParser &req);
std::string to_string_c98(size_t val);
