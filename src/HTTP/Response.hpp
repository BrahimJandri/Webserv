#pragma once

#include <string>
#include <map>
#include <vector> // Potentially useful for internal use, though not strictly required for the basics here
#include "Client.hpp"
#include "CGIHandler.hpp" // Include the CGI handler for handling CGI requests
#include "Request.hpp" // Include the request parser to handle incoming requests


// Forward declaration for C++98 compatibility if not defined globally.
// This function might be needed if you convert numbers to strings within Response methods.
std::string to_string_c98(size_t val); // Assuming this utility function is available

class Response {
private:
    int statusCode;
	std::string statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _httpVersion; // Typically "HTTP/1.1"
    CGIHandler cgiHandler;


public:
    // Constructor and Destructor
    Response();
    ~Response();

    // Setters
  	void setStatus(int code, const std::string& message);
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    void setHttpVersion(const std::string& version);

    // Getters (const correctness is important)
    int getStatusCode() const;
    const std::string& getStatusText() const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::string& getBody() const;
    const std::string& getHttpVersion() const;

    void handleCGI(const requestParser &request);
    std::string toString() const;
};
