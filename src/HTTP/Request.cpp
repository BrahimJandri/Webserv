// src/HTTP/Request.cpp
#include "Request.hpp"
#include <sstream>	 // For parsing
#include <algorithm> // For std::find, std::min etc.
#include <cstddef>	 // For size_t

requestParser::requestParser() : _method(""), _path(""), _httpVersion(""), _body("")
{
	// Initialize other members if necessary
}

requestParser::~requestParser()
{
	// Destructor logic
}

// Implementation for the new getBody() method
const std::string &requestParser::getBody() const
{
	return _body;
}

// CRITICAL: Update your parseRequest method
// This is a simplified example. Your actual parseRequest logic might be more robust.
// The key is to find the double CRLF (\r\n\r\n) that separates headers from the body.
void requestParser::parseRequest(const std::string &rawRequest)
{
	// Clear previous state
	_method.clear();
	_path.clear();
	_httpVersion.clear();
	_headers.clear();
	_body.clear();

	// Find the end of headers (double CRLF)
	size_t header_end_pos = rawRequest.find("\r\n\r\n");

	// If no double CRLF is found, it's an incomplete request or only a header section.
	// For simplicity, we'll assume no body if not found.
	if (header_end_pos == std::string::npos)
	{
		// Handle malformed request or request without body
		// For now, parse headers and set body to empty
		std::istringstream iss_headers(rawRequest);
		std::string line;

		// Parse Request Line (Method Path HTTP-Version)
		if (std::getline(iss_headers, line) && !line.empty())
		{
			std::istringstream line_iss(line);
			line_iss >> _method >> _path >> _httpVersion;
		}

		// Parse Headers
		while (std::getline(iss_headers, line) && line != "\r" && !line.empty()) // Loop until empty line or end of stream
		{
			// Remove trailing \r if present (for Windows-style CRLF)
			// C++98 way to remove last character:
			if (!line.empty() && line[line.length() - 1] == '\r')
			{
				line = line.substr(0, line.length() - 1); // Use substr to remove last char
			}

			size_t colon_pos = line.find(':');
			if (colon_pos != std::string::npos)
			{
				std::string key = line.substr(0, colon_pos);
				std::string value = line.substr(colon_pos + 1);
				// Trim whitespace from key and value
				key.erase(0, key.find_first_not_of(" \t"));
				key.erase(key.find_last_not_of(" \t") + 1);
				value.erase(0, value.find_first_not_of(" \t"));
				value.erase(value.find_last_not_of(" \t") + 1);
				_headers[key] = value;
			}
		}
		_body.clear(); // No body if double CRLF not found
		return;
	}

	// Extract header part and body part
	std::string header_part = rawRequest.substr(0, header_end_pos);
	_body = rawRequest.substr(header_end_pos + 4); // +4 for \r\n\r\n

	std::istringstream iss_headers(header_part);
	std::string line;

	// Parse Request Line (Method Path HTTP-Version)
	if (std::getline(iss_headers, line) && !line.empty())
	{
		std::istringstream line_iss(line);
		line_iss >> _method >> _path >> _httpVersion;
	}

	// Parse Headers
	while (std::getline(iss_headers, line) && line != "\r" && !line.empty())
	{
		// Remove trailing \r if present (for Windows-style CRLF)
		if (!line.empty() && line[line.length() - 1] == '\r')
		{
			line = line.substr(0, line.length() - 1); // Use substr
		}

		size_t colon_pos = line.find(':');
		if (colon_pos != std::string::npos)
		{
			std::string key = line.substr(0, colon_pos);
			std::string value = line.substr(colon_pos + 1);
			// Trim whitespace from key and value
			key.erase(0, key.find_first_not_of(" \t"));
			key.erase(key.find_last_not_of(" \t") + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of(" \t") + 1);
			_headers[key] = value;
		}
	}
}

// Your other getter implementations would go here
const std::string &requestParser::getMethod() const { return _method; }
const std::string &requestParser::getPath() const { return _path; }
const std::string &requestParser::getHttpVersion() const { return _httpVersion; }
const std::map<std::string, std::string> &requestParser::getHeaders() const { return _headers; }