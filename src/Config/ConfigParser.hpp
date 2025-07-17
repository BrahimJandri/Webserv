#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>

class ConfigParser {
public:
    struct LocationConfig {
        std::string path;
        std::string root;
        std::vector<std::string> index;
        std::vector<std::string> allowed_methods;
        std::string cgi_extension;
        std::string cgi_path;
        std::string return_directive;
        bool autoindex;
        
        LocationConfig();
    };
    
    struct Listen {
        std::string host;
        std::string port;
        
        Listen();
        Listen(const std::string& h, const std::string& p);
    };
    
    struct ServerConfig {
        std::vector<Listen> listen;
        std::string server_name;
        std::map<std::string, std::string> error_pages;
        std::string limit_client_body_size;
        bool autoindex;
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
    void parseLocation(LocationConfig& location);
    void parseServer(ServerConfig& server);
    Listen parseListen(const std::string& listen_value);
    std::string intToString(int value);
    void validatePorts();
    bool isAtBlockBoundary();
    

    ConfigParser();
    void parseFile(const std::string& filename);
    void parseString(const std::string& config_content);
    void parse();
    void printConfig();
    const std::vector<ServerConfig>& getServers() const;

    size_t  getListenCount() const;  

    size_t getServerCount() const;
};