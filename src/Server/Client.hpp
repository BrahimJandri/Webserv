#pragma once

#include <string>
#include "../HTTP/Request.hpp"
// #include "../HTTP/Response.hpp"

class Client
{
private:
	int _fd;				// Client socket file descriptor
	std::string _buffer;	// Buffer for incoming data
	bool _request_complete; // Flag to indicate if the request is complete
	bool _response_ready;	// Flag to indicate if the response is ready to be sent
	requestParser _request; // Parsed request object
	std::string _response;	// Response to be sent

public:
	Client();
	Client(int fd);
	~Client();

	int getFd() const;
	bool isRequestComplete() const;
	bool isResponseReady() const;

	void appendToBuffer(const std::string &data);
	bool processRequest();
	void prepareResponse();
	std::string getResponse() const;
	void clearResponse();

	const requestParser &getRequest() const;
	std::string to_string_client(size_t val);
};
