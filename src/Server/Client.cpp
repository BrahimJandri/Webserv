#include "Client.hpp"
#include "../Utils/Logger.hpp"
#include "../HTTP/ResponseBuilder.hpp" // Add this include
#include <iostream>
#include <sstream>
#include <cctype> // For std::tolower

static std::map<std::string, std::pair<std::string, std::string> > users;

std::map<std::string, std::string> parseUrlEncodedBody(const std::string &body)
{
	std::map<std::string, std::string> data;
	std::istringstream stream(body);
	std::string pair;
	while (std::getline(stream, pair, '&'))
	{
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos)
		{
			std::string key = pair.substr(0, eq_pos);
			std::string value = pair.substr(eq_pos + 1);
			data[key] = value;
		}
	}
	return data;
}

// Helper function for creating HTTP responses
std::string Client::to_string_client(size_t val)
{
	std::ostringstream oss;
	oss << val;
	return oss.str();
}

Client::Client() : _fd(-1), _request_complete(false), _response_ready(false)
{
}

Client::Client(int fd) : _fd(fd), _request_complete(false), _response_ready(false)
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

bool Client::processRequest()
{
	if (_request_complete)
	{
		return true;
	}

	// First, check if we have complete headers (double CRLF)
	size_t header_end_pos = _buffer.find("\r\n\r\n");
	if (header_end_pos == std::string::npos)
	{
		// Headers not complete yet
		return false;
	}

	// Extract just the header part to check Content-Length
	std::string header_part = _buffer.substr(0, header_end_pos);

	// Look for Content-Length header (case-insensitive)
	size_t content_length = 0;
	bool has_content_length = false;

	std::istringstream header_stream(header_part);
	std::string line;

	// Skip the request line (first line)
	if (std::getline(header_stream, line))
	{
		// Parse remaining header lines
		while (std::getline(header_stream, line) && !line.empty())
		{
			// Remove trailing \r if present
			if (!line.empty() && line[line.length() - 1] == '\r')
			{
				line = line.substr(0, line.length() - 1);
			}

			size_t colon_pos = line.find(':');
			if (colon_pos != std::string::npos)
			{
				std::string key = line.substr(0, colon_pos);
				std::string value = line.substr(colon_pos + 1);

				// Trim whitespace
				key.erase(0, key.find_first_not_of(" \t"));
				key.erase(key.find_last_not_of(" \t") + 1);
				value.erase(0, value.find_first_not_of(" \t"));
				value.erase(value.find_last_not_of(" \t") + 1);

				// Case-insensitive comparison for Content-Length
				std::string key_lower = key;
				for (size_t i = 0; i < key_lower.length(); ++i)
				{
					key_lower[i] = std::tolower(key_lower[i]);
				}

				if (key_lower == "content-length")
				{
					// Convert value to size_t
					std::istringstream iss(value);
					iss >> content_length;
					has_content_length = true;
					break;
				}
			}
		}
	}

	// Calculate how much body data we currently have
	size_t body_start = header_end_pos + 4; // +4 for "\r\n\r\n"
	size_t current_body_length = 0;
	if (body_start < _buffer.length())
	{
		current_body_length = _buffer.length() - body_start;
	}

	// Check if we have the complete request
	if (has_content_length)
	{
		// For POST/PUT requests, we need the complete body
		if (current_body_length >= content_length)
		{
			// We have the complete request!
			_request.parseRequest(_buffer);
			_request_complete = true;
			return true;
		}
		else
		{
			// Still waiting for more body data
			return false;
		}
	}
	else
	{
		// No Content-Length header (GET requests, etc.)
		_request.parseRequest(_buffer);
		_request_complete = true;
		return true;
	}
}

void Client::prepareResponse()
{
	if (_response_ready)
	{
		return;
	}

	// Use the actual ResponseBuilder to handle the request based on method
	Response response;
	std::string docRoot = "www"; // Default document root

	if (_request.getPath() == "/signup" && _request.getMethod() == "POST")
	{
		std::map<std::string, std::string> body = parseUrlEncodedBody(_request.getBody());
		if (users.count(body["email"]))
			response = ResponseBuilder::buildErrorResponse(409, "Conflict: User already exists");
		else
		{
			users[body["email"]] = std::make_pair(body["password"], body["name"]);
			response.setStatus(200, "OK");
			response.addHeader("Set-Cookie", CookieManager::createSetCookieHeader("username", body["name"], 3600));
			std::cout << "New user signed up: " << body["name"] << std::endl;
		}
	}
	else if (_request.getPath() == "/login" && _request.getMethod() == "POST")
	{
		std::map<std::string, std::string> body = parseUrlEncodedBody(_request.getBody());
		if (users.count(body["email"]) && users[body["email"]].first == body["password"])
		{
			std::string username = users[body["email"]].second;
			response.setStatus(200, "OK");
			response.addHeader("Set-Cookie", CookieManager::createSetCookieHeader("username", username, 86400));
			std::cout << "User logged in: " << username << std::endl;
		}
		else
			response = ResponseBuilder::buildErrorResponse(401, "Unauthorized");
	}
	else if ((_request.getMethod() == "GET" || _request.getMethod() == "POST") && _request.getPath().find("/cgi-bin/") != std::string::npos)
		response.handleCGI(_request);
	else if (_request.getMethod() == "GET")
	{
		response = ResponseBuilder::buildGetResponse(_request, docRoot);
	}
	else if (_request.getMethod() == "POST")
	{
		response = ResponseBuilder::buildPostResponse(_request, docRoot);
	}
	else if (_request.getMethod() == "DELETE")
	{
		response = ResponseBuilder::buildDeleteResponse(_request, docRoot);
	}
	else
	{
		response = ResponseBuilder::buildErrorResponse(405, "Method Not Allowed");
	}

	_response = response.toString();
	_response_ready = true;
}

std::string Client::getResponse() const
{
	return _response;
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
