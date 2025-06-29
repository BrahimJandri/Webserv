#include "Response.hpp"
#include <sstream> // For std::ostringstream and to_string_c98
#include <algorithm> // For general utility if needed, not strictly for current methods


// Constructor
Response::Response() : statusCode(200), statusMessage("OK"), _httpVersion("HTTP/1.1") {
	addHeader("Server", "Webserv/1.0");
	addHeader("Connection", "close");
}
// Destructor
Response::~Response() {
    // No dynamic memory allocated directly within Response object that needs explicit deletion,
    // as std::string and std::map handle their own memory.
}

void Response::setStatus(int code, const std::string& message) {
	statusCode = code;
	statusMessage = message;
}

void Response::addHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void Response::setBody(const std::string& body) {
    _body = body;
}

void Response::setHttpVersion(const std::string& version) {
    _httpVersion = version;
}

// Getters
int Response::getStatusCode() const {
    return statusCode;
}

const std::string& Response::getStatusText() const {
    return statusMessage;
}

const std::map<std::string, std::string>& Response::getHeaders() const {
    return _headers;
}

const std::string& Response::getBody() const {
    return _body;
}

const std::string& Response::getHttpVersion() const {
    return _httpVersion;
}

// toString method to assemble the full HTTP response
std::string Response::toString() const {
    std::ostringstream oss;

    // Status Line: HTTP-Version Status-Code Reason-Phrase CR LF
    oss << _httpVersion << " " << statusCode << " " << statusMessage << "\r\n";

    // Headers: Header-Name: Header-Value CR LF
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }

    // Always add Content-Length if a body exists and header isn't already set
    // (This ensures compliance, though CGIHandler explicitly adds it)
    if (_body.length() > 0 && _headers.find("Content-Length") == _headers.end()) {
        oss << "Content-Length: " << to_string_c98(_body.length()) << "\r\n";
    }

    // End of headers: CR LF
    oss << "\r\n";

    // Body
    oss << _body;

    return oss.str();
}
