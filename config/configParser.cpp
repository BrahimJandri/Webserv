#include "parserHeader.hpp"

ConfigParser::ConfigParser(const std::string &file_path)
    : _file_path(file_path) {}

const std::vector<ServerConfig> &ConfigParser::getServers() const
{
    return _servers;
}

void ConfigParser::_trim(std::string &line)
{
    size_t start = line.find_first_not_of(" \t");
    size_t end = line.find_last_not_of(" \t");
    if (start == std::string::npos || end == std::string::npos)
        line = "";
    else
        line = line.substr(start, end - start + 1);
}

void ConfigParser::_parseServerBlock(std::ifstream &file)
{
    ServerConfig server;
    std::string line;
    bool inside = false;

    while (std::getline(file, line))
    {
        _trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        if (line == "server {")
        {
            inside = true;
            continue;
        }
        if (line == "}")
            break;

        std::istringstream iss(line);
        std::string directive;
        iss >> directive;

        if (directive == "port")
        {
            std::string value;
            iss >> value;
            size_t colon = value.find(':');
            if (colon != std::string::npos)
            {
                server.host = value.substr(0, colon);
                server.port = std::atoi(value.substr(colon + 1).c_str());
            }
            else
            {
                server.host = "0.0.0.0";
                server.port = std::atoi(value.c_str());
            }
        }
        else if (directive == "server_name")
        {
            iss >> server.server_name;
        }
        else if (directive == "location")
        {
            Location loc;
            iss >> loc.path;
            std::getline(file, line); // skip {
            while (std::getline(file, line))
            {
                _trim(line);
                if (line == "}")
                    break;
                std::istringstream loc_iss(line);
                std::string loc_dir;
                loc_iss >> loc_dir;
                if (loc_dir == "root")
                    loc_iss >> loc.root;
                else if (loc_dir == "index")
                    loc_iss >> loc.index;
            }
            server.locations.push_back(loc);
        }
        else if (directive == "error_page")
        {
            int code;
            std::string path;
            iss >> code >> path;
            server.error_pages[code] = path;
        }
    }
    _servers.push_back(server);
}

bool ConfigParser::parse()
{
    std::ifstream file(_file_path.c_str());
    if (!file.is_open())
    {
        std::cerr << "Could not open config file: " << _file_path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        _trim(line);
        if (line == "server {")
            _parseServerBlock(file);
    }

    return true;
}
