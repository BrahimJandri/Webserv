#pragma once

#include <map>
#include <sstream>	 // For parsing


class requestParser
{
private:
    std::string _method;
    std::string _path;
    std::string _httpVersion;
    std::map<std::string, std::string> _headers;
    std::string _body;

public:
    // Constructor (example, adjust as per your actual constructor)
    requestParser();
    ~requestParser();

    // Your existing parseRequest method (ensure it populates _body)
    void parseRequest(const std::string &rawRequest);

    // Existing getters
    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::string& getHttpVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;

    // --- ADD THIS NEW GETTER ---
    const std::string& getBody() const; // New: Getter for the request body

    // You might also need a setter for the body if parsing happens in stages
    void setBody(const std::string& body);
};