#pragma once

#include <string>
#include <iostream>
#include "../Request/Request.hpp"

class Client
{
private:
    int _fd;
    std::string _buffer;
    bool _request_complete;
    bool _response_ready;
    requestParser _request;
    std::string _response;
    size_t _content_length;

public:
    Client();
    Client(int fd);
    ~Client();

    int getFd() const;
    bool isRequestComplete() const;
    bool isResponseReady() const;

    void appendToBuffer(const std::string &data);
    bool processRequest();
    std::string getResponse() const;
    void clearResponse();
    const requestParser &getRequest() const;
    void setResponse(const std::string &response);
    std::string to_string_client(size_t val);
};
