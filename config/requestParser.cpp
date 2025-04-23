#include "parserHeader.hpp"

HttpRequest parse_request(const std::string &raw_request)
{
    std::istringstream stream(raw_request);
    std::string line;

    HttpRequest request;

    // Parse request line
    if (std::getline(stream, line))
    {
        std::istringstream request_line(line);
        request_line >> request.method >> request.path >> request.http_version;
    }

    // Parse headers
    while (std::getline(stream, line) && line != "\r")
    {
        if (!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);

        size_t colon = line.find(": ");
        if (colon != std::string::npos)
        {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);
            request.headers[key] = value;
        }
    }

    return request;
}