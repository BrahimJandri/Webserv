#pragma once

#include <iostream>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include <map>
#include <set>
#include <algorithm>
#include "../Utils/AnsiColor.hpp"
#include "../Utils/Logger.hpp"

class ConfigParser
{
public:
    struct LocationConfig
    {
        std::string path;
        std::string root;
        std::string index;
        std::vector<std::string> allowed_methods;
        std::map<std::string, std::string> cgi;
        std::string return_directive;
        bool autoindex;

        LocationConfig();
    };

    struct Listen
    {
        std::string host;
        std::string port;

        Listen();
        Listen(const std::string &h, const std::string &p);
    };

    struct ServerConfig
    {
        std::vector<Listen> listen;
        std::string server_name;
        std::string root;
        std::map<int, std::string> error_pages;
        size_t limit_client_body_size;
        std::vector<LocationConfig> locations;

        ServerConfig();
    };

    std::vector<ServerConfig> servers;
    std::string content;
    size_t pos;
    int line_number;

    void skipWhitespace();
    void skipComments();
    std::string parseToken();
    char parseSpecialChar();
    std::string parseDirectiveValue();
    std::vector<std::string> parseMultipleValues();
    bool expectSemicolon();
    void parseLocation(LocationConfig &location);
    void parseServer(ServerConfig &server);
    Listen parseListen(const std::string &listen_value);
    std::string intToString(int value);
    void validatePorts();
    void validateRequiredDirectives();
    bool isAtBlockBoundary();

    ConfigParser();
    void parseFile(const std::string &filename);
    void parseString(const std::string &config_content);
    void parse();
    void printConfig();
    const std::vector<ServerConfig> &getServers() const;

    size_t getListenCount() const;
    size_t parseSizeToBytes(const std::string &input);

    size_t getServerCount() const;

    void   printPos();


    //listen
    bool    isValidIPv4(const std::string& ip);
    bool    isDigitString(const std::string& str);
    bool    isValidPort(const std::string& port);

    bool    isValidServerName(const std::string& name);

};
