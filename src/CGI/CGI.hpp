#pragma once

#include <sys/wait.h>
#include <cstring>
#include "../Request/Request.hpp"   // Assuming requestParser is defined in this header
#include "../Response/Response.hpp" // For Response class
#include "../Client/Client.hpp" // For Client class
class Request; // Forward declaration if you want to use Request in prepareCGIEnv

class CGIHandler
{
public:
    CGIHandler();
    ~CGIHandler();

    // Execute the CGI script with given env and request body (for POST)
    std::string execute(const std::string &scriptPath, const requestParser &request, const std::map<std::string, std::string> &envVars, const std::string &interpreter);
    std::string handleCGI(const std::string &scriptPath, const requestParser &request, std::string interpreter, int client_fd);
    static std::map<std::string, std::string> prepareCGIEnv(const requestParser &request);

private:
    char **buildEnvArray(const std::map<std::string, std::string> &envVars);
    void freeEnvArray(char **env);
};

char to_cgi_char(char c);