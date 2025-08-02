#pragma once

#include <map>
#include <sstream>


class requestParser
{
private:
    std::string _method;
    std::string _path;
    std::string _httpVersion;
    std::map<std::string, std::string> _headers;
    std::string _body;

public:
    requestParser();
    ~requestParser();

    void parseRequest(const std::string &rawRequest);

    // Existing getters
    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::string& getHttpVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;

    const std::string& getBody() const;
    void setBody(const std::string& body);
};