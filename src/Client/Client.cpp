#include "Client.hpp"

Client::Client() : _fd(-1), _request_complete(false), _response_ready(false), _content_length(0)
{
}

Client::Client(int fd) : _fd(fd), _request_complete(false), _response_ready(false), _content_length(0)
{
}

Client::~Client()
{
}

int Client::getFd() const
{
    return _fd;
}

bool Client::isRequestComplete() const
{
    return _request_complete;
}

bool Client::isResponseReady() const
{
    return _response_ready;
}

void Client::appendToBuffer(const std::string &data)
{
    _buffer += data;
}

bool Client::processRequest() // brahim
{
    if (_request_complete)
    {
        return true;
    }

    size_t header_end_pos = _buffer.find("\r\n\r\n");
    if (header_end_pos == std::string::npos)
    {
        return false;
    }

    std::string header_part = _buffer.substr(0, header_end_pos);

    bool has_content_length = false;

    std::istringstream header_stream(header_part);
    std::string line;

    if (std::getline(header_stream, line))
    {
        while (std::getline(header_stream, line) && !line.empty())
        {
            if (!line.empty() && line[line.length() - 1] == '\r')
            {
                line = line.substr(0, line.length() - 1);
            }

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);

                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                std::string key_lower = key;
                for (size_t i = 0; i < key_lower.length(); ++i)
                {
                    key_lower[i] = std::tolower(key_lower[i]);
                }

                if (key_lower == "content-length")
                {
                    std::istringstream iss(value);
                    iss >> _content_length;
                    has_content_length = true;
                    break;
                }
            }
        }
    }

    size_t body_start = header_end_pos + 4; // +4 for "\r\n\r\n"
    size_t current_body_length = 0;
    if (body_start < _buffer.length())
    {
        current_body_length = _buffer.length() - body_start;
    }

    if (has_content_length)
    {
        if (current_body_length >= _content_length)
        {
            _request.parseRequest(_buffer);
            _request_complete = true;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        _request.parseRequest(_buffer);
        _request_complete = true;
        return true;
    }
}

std::string Client::getResponse() const
{
    return _response;
}

void Client::setResponse(const std::string &response)
{
    _response = response;
}

void Client::clearResponse()
{
    _response.clear();
    _response_ready = false;
    _request_complete = false;
    _buffer.clear();
}

const requestParser &Client::getRequest() const
{
    return _request;
}

std::string Client::to_string_client(size_t val)
{
    std::ostringstream oss;
    oss << val;
    return oss.str();
}
