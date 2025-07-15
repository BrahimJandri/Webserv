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
private:
    struct Location {
        std::string path;
        std::string root;
        std::vector<std::string> index;
        std::vector<std::string> allowed_methods;
        std::string cgi_extension;
        std::string cgi_path;
        std::string return_directive;
        bool autoindex;
        
        Location();
    };
    
    struct Listen {
        std::string host;
        std::string port;
        
        Listen();
        Listen(const std::string& h, const std::string& p);
    };
    
    struct Server {
        std::vector<Listen> listen;
        std::string server_name;
        std::map<std::string, std::string> error_pages;
        std::string limit_client_body_size;
        bool autoindex;
        std::vector<Location> locations;
        
        Server();
    };
    
    std::vector<Server> servers;
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
    void parseLocation(Location& location);
    void parseServer(Server& server);
    Listen parseListen(const std::string& listen_value);
    std::string intToString(int value);
    void validatePorts();
    bool isAtBlockBoundary();
    
public:
    ConfigParser();
    void parseFile(const std::string& filename);
    void parseString(const std::string& config_content);
    void parse();
    void printConfig();
    const std::vector<Server>& getServers() const;
};