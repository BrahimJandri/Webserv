#include "Response.hpp"
#include <sstream> // for std::ostringstream
#include <cstdlib> // for itoa alternative

Response::Response()
    : statusCode(200), statusMessage("OK")
{
    // Default content-type
    headers["Content-Type"] = "text/plain";
}

Response::~Response() {}

void Response::setStatus(int code, const std::string &message)
{
    statusCode = code;
    statusMessage = message;
}

void Response::addHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

void Response::setBody(const std::string &content)
{
    body = content;

    // Set Content-Length header
    std::ostringstream oss;
    oss << body.size();
    headers["Content-Length"] = oss.str();
}

std::string Response::toString() const
{
    std::ostringstream oss;

    // Status line
    oss << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";

    // Headers
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }

    // End of headers
    oss << "\r\n";

    // Body
    oss << body;

    return oss.str();
}
