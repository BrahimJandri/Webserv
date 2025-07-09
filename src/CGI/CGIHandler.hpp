#pragma once

#include <algorithm> // for std::transform
#include <string>
#include <map>
#include <unistd.h>

#include <sys/wait.h>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include "../HTTP/Request.hpp" // Assuming requestParser is defined in this header
#include "../Server/Server.hpp" // For to_string_c98 if needed

class Request; // Forward declaration if you want to use Request in prepareCGIEnv

class CGIHandler
{
public:
    CGIHandler();
    ~CGIHandler();

    // Execute the CGI script with given env and request body (for POST)
    std::string execute(const std::string &scriptPath,
                        const std::string &requestMethod,
                        const std::string &requestBody,
                        const std::map<std::string, std::string> &envVars);

    static std::map<std::string, std::string> prepareCGIEnv(const requestParser &request);

private:
    char **buildEnvArray(const std::map<std::string, std::string> &envVars);
    void freeEnvArray(char **env);
    std::string getInterpreterForScript(const std::string &scriptPath);
};


char to_cgi_char(char c);