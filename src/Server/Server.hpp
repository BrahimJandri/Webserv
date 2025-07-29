#pragma once

#include <dirent.h> // opendit
#include <sys/epoll.h>
#include <arpa/inet.h> // sockaddr
#include <fcntl.h>     // F_GETFL ..
#include <sys/stat.h>
#include <netdb.h> // accept
#include "./Parser/ConfigParser.hpp"
#include "./Client/Client.hpp"
#include "../Request/Request.hpp"
#include "../Response/Response.hpp"
#include "../Utils/AnsiColor.hpp"
#include "../Utils/Logger.hpp"

class Server
{
private:
    int epoll_fd;
    std::map<int, Client *> clients;
    static const int MAX_EVENTS = 64;
    std::set<int> server_fds;
    std::map<int, std::vector<ConfigParser::ServerConfig> > serverConfigMap; // EDITED BY RACHID
    std::map<int, ConfigParser::ServerConfig> clientToServergMap;

public:
    Server();
    ~Server();

    static volatile sig_atomic_t _turnoff;

    void setupServers(const ConfigParser &parser);
    void handleConnections();
    void acceptNewConnection(int server_fd);
    void handleClientRead(int client_fd);
    void handleClientWrite(int client_fd);
    void closeClientConnection(int client_fd);
    int prepareResponse(const requestParser &req, int client_fd);
    void Cleanup();
};

int create_server_socket(const std::string &host, int port);
std::string to_string_c98(size_t val);
void send_error_response(int client_fd, int status_code, const std::string &message, const ConfigParser::ServerConfig &serverConfig);

void send_http_headers(int client_fd, const std::string &status_line,
                       const std::string &content_type, size_t content_length,
                       const std::string &connection_header);
bool isDirectory(const std::string &path);
std::string generate_autoindex(const std::string &dir_path, const std::string &uri);
const ConfigParser::LocationConfig *findMatchingLocation(const std::vector<ConfigParser::LocationConfig> &locations, const std::string &requestPath);
void normalize_path(std::string &path);
std::string get_file_extension(const std::string &filename);