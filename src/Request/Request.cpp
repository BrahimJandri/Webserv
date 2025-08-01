#include "Request.hpp"

requestParser::requestParser() : _method(""), _path(""), _httpVersion(""), _body("") {}

requestParser::~requestParser() {}

const std::string &requestParser::getBody() const
{
	return _body;
}

void requestParser::setBody(const std::string &body)
{
	_body = body;
}

void requestParser::parseRequest(const std::string &rawRequest)
{
	_method.clear();
	_path.clear();
	_httpVersion.clear();
	_headers.clear();
	_body.clear();

	size_t header_end_pos = rawRequest.find("\r\n\r\n");

	if (header_end_pos == std::string::npos)
	{
		std::istringstream iss_headers(rawRequest);
		std::string line;

		if (std::getline(iss_headers, line) && !line.empty())
		{
			std::istringstream line_iss(line);
			line_iss >> _method >> _path >> _httpVersion;
		}

		while (std::getline(iss_headers, line) && line != "\r" && !line.empty()) // Loop until empty line or end of stream
		{
			if (!line.empty() && line[line.length() - 1] == '\r')
			{
				line = line.substr(0, line.length() - 1); // Use substr to remove last char
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
				_headers[key] = value;
			}
		}
		_body.clear(); // No body if double CRLF not found
		return;
	}

	std::string header_part = rawRequest.substr(0, header_end_pos);
	_body = rawRequest.substr(header_end_pos + 4); // +4 for \r\n\r\n

	std::istringstream iss_headers(header_part);
	std::string line;

	if (std::getline(iss_headers, line) && !line.empty())
	{
		std::istringstream line_iss(line);
		line_iss >> _method >> _path >> _httpVersion;
	}

	while (std::getline(iss_headers, line) && line != "\r" && !line.empty())
	{
		if (!line.empty() && line[line.length() - 1] == '\r')
		{
			line = line.substr(0, line.length() - 1); // Use substr
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
			_headers[key] = value;
		}
	}
}

const std::string &requestParser::getMethod() const { return _method; }
const std::string &requestParser::getPath() const { return _path; }
const std::string &requestParser::getHttpVersion() const { return _httpVersion; }
const std::map<std::string, std::string> &requestParser::getHeaders() const { return _headers; }