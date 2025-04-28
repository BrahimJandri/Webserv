#include "Client.hpp"
#include "../Utils/Logger.hpp"
#include <iostream>
#include <sstream>

// Helper function for creating HTTP responses
std::string to_string_client(size_t val) {
	std::ostringstream oss;
	oss << val;
	return oss.str();
}

Client::Client() : _fd(-1), _request_complete(false), _response_ready(false) {
}

Client::Client(int fd) : _fd(fd), _request_complete(false), _response_ready(false) {
}

Client::~Client() {
}

int Client::getFd() const {
	return _fd;
}

bool Client::isRequestComplete() const {
	return _request_complete;
}

bool Client::isResponseReady() const {
	return _response_ready;
}

void Client::appendToBuffer(const std::string &data) {
	_buffer += data;
}

bool Client::processRequest() {
	if (_request_complete) {
		return true;
	}

	// Check if we have a complete HTTP request
	if (_buffer.find("\r\n\r\n") != std::string::npos) {
		_request.parseRequest(_buffer);
		_request_complete = true;
		return true;
	}

	return false;
}

void Client::prepareResponse() {
	if (_response_ready) {
		return;
	}

	// Simple response for now - this would be replaced with proper response handling
	std::string body = "<h1>Hello from Webserv</h1>";
	_response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: " + to_string_client(body.length()) + "\r\n"
		"\r\n" + 
		body;

	_response_ready = true;
}

std::string Client::getResponse() const {
	return _response;
}

void Client::clearResponse() {
	_response.clear();
	_response_ready = false;
	_request_complete = false;
	_buffer.clear();
}

const requestParser &Client::getRequest() const {
	return _request;
}
