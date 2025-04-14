#pragma once

#include <iostream>
#include <vector>

struct LocationConfig
{
    std::string path;
    std::string method;
    std::string root;
};

struct ServerConfig {
    int port;
    std::string server_name;
    std::string root;
    std::vector<LocationConfig> locations;
};


std::vector<ServerConfig> parse_config(const std::string &path);