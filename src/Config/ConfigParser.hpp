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
#include <algorithm>
#include "../HTTP/Request.hpp"

class ConfigValue;
class LocationConfig;
class ServerConfig;
class ConfigParser;

// Helper functions
std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, char delimiter);
std::vector<std::string> splitWhitespace(const std::string &str);
bool isNumber(const std::string &str);
int parseSize(const std::string &str);

// Class to hold different types of configuration values
class ConfigValue
{
private:
    std::string stringValue;
    int intValue;
    bool boolValue;
    std::vector<std::string> listValue;
    std::map<std::string, std::string> mapValue;

public:
    enum Type
    {
        STRING,
        INT,
        BOOL,
        LIST,
        MAP
    };
    Type type;

    // Constructors
    ConfigValue();
    ConfigValue(const std::string &val);
    ConfigValue(int val);
    ConfigValue(bool val);
    ConfigValue(const std::vector<std::string> &val);
    ConfigValue(const std::map<std::string, std::string> &val);

    // Getters
    std::string asString() const;
    int asInt() const;
    bool asBool() const;
    std::vector<std::string> asList() const;
    std::map<std::string, std::string> asMap() const;
    // Print method
    void print() const;
};

// Class to hold location configuration
class LocationConfig
{
public:
    std::string path;
    std::map<std::string, ConfigValue> directives;

    LocationConfig();
    LocationConfig(const std::string &p);

    void addDirective(const std::string &key, const ConfigValue &value);

    ConfigValue getDirective(const std::string &key) const;

    void print() const;
};

// Class to hold server configuration
class ServerConfig
{
public:
    std::map<std::string, ConfigValue> directives;
    std::map<std::string, LocationConfig> locations;

    ServerConfig();
    void addDirective(const std::string &key, const ConfigValue &value);

    void addLocation(const LocationConfig &location);

    ConfigValue getDirective(const std::string &key) const;

    LocationConfig getLocation(const std::string &path) const;

    void print() const;
};

// Main parser class
class ConfigParser
{
private:
    std::string filename;
    std::vector<std::string> lines;
    std::vector<ServerConfig> serverConfigs; // Changed to vector for multiple servers

    void readFile();

    ConfigValue parseDirectiveValue(const std::string &key, const std::string &value);

    int findClosingBrace(size_t startIndex);

    LocationConfig parseLocationBlock(size_t startIndex);

    std::pair<std::string, std::string> parseDirectiveLine(const std::string &line);

    ServerConfig parseServerBlock(size_t startIndex);

public:
    ConfigParser();
    ConfigParser(const std::string &file);

    void parse();

    const std::vector<ServerConfig> &getServerConfigs() const;

    const ServerConfig &getServerConfig(size_t index = 0) const;

    size_t getServerCount() const;

    void printConfig() const;
};

std::string trim(const std::string &str);

std::vector<std::string> split(const std::string &str, char delimiter);

std::vector<std::string> splitWhitespace(const std::string &str);

bool isNumber(const std::string &str);

int parseSize(const std::string &str);

int create_server_socket(const std::string &host, int port);
void handle_requests(int server_fd, requestParser &req);
std::string to_string_c98(size_t val);
