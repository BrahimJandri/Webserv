#include "webserv.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<ServerConfig> parse_config(const std::string &path)
{
    std::vector<ServerConfig> servers;
    std::ifstream file(path.c_str());
    std::string line;

    if (!file.is_open())
    {
        std::cerr << "❌ Failed to open config file: " << path << std::endl;
        return servers;
    }

    ServerConfig server;
    LocationConfig location;
    bool in_server_block = false;
    bool in_location_block = false;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "server")
        {
            in_server_block = true;
            continue;
        }
        else if (token == "location")
        {
            in_location_block = true;
            iss >> location.path;
            continue;
        }
        else if (token == "}")
        {
            if (in_location_block)
            {
                server.locations.push_back(location);
                location = LocationConfig();
                in_location_block = false;
            }
            else if (in_server_block)
            {
                servers.push_back(server);
                server = ServerConfig();
                in_server_block = false;
            }
        }

        std::string value;
        iss >> value;

        if (in_location_block)
        {
            if (token == "method")
                location.method = value;
            else if (token == "root")
                location.root = value;
        }
        else if (in_server_block)
        {
            if (token == "listen")
                server.port = std::stoi(value);
            else if (token == "server_name")
                server.server_name = value;
            else if (token == "root")
                server.root = value;
        }
    }

    return servers;
}
