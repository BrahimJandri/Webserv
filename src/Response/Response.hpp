#pragma once

#include "../Request/Request.hpp"
#include "../CGI/CGI.hpp"
#include "../Server/Server.hpp"
#include "../Parser/ConfigParser.hpp"

#include <sstream>   // For std::ostringstream and to_string_c98
#include <algorithm> // For general utility if needed, not strictly for current methods

class Response
{
private:
    int statusCode;
    std::string statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _httpVersion; // Typically "HTTP/1.1"

public:
    // Constructor and Destructor
    Response();
    ~Response();

    // Setters
    void setStatus(int code, const std::string &message);
    void addHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);
    void setHttpVersion(const std::string &version);

    // Getters (const correctness is important)
    int getStatusCode() const;
    const std::string &getStatusText() const;
    const std::map<std::string, std::string> &getHeaders() const;
    const std::string &getBody() const;
    const std::string &getHttpVersion() const;

    void handleCGI(const std::string &scriptPath, const requestParser &request, std::string interpret);
    std::string toString() const;
    static Response buildFileResponse(const std::string &filePath, const ConfigParser::ServerConfig &serverConfig, int client_fd);

    static std::string getFileExtension(const std::string &filePath);
    static bool fileExists(const std::string &filePath);
    static std::string getContentType(const std::string &filePath);
    static Response buildAutoindexResponse(const std::string &htmlContent);

    static Response buildGetResponse(const requestParser &request, const std::string &docRoot, const bool autoIndex, int client_fd, const ConfigParser::ServerConfig &serverConfig);
    static Response buildPostResponse(const requestParser &request, const std::string &docRoot, int client_fd, const ConfigParser::ServerConfig &serverConfig);
    static Response buildDeleteResponse(const requestParser &request, const std::string &docRoot, int client_fd, const ConfigParser::ServerConfig &serverConfig);
};
