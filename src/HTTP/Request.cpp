#include "Request.hpp"

requestParser::requestParser()
{
}

requestParser::~requestParser()
{
}

void requestParser::parseRequest(const std::string &raw_request)
{
	std::istringstream stream(raw_request);
	std::string line;

	// Parse the request line
	if (std::getline(stream, line))
	{
		std::istringstream request_line(line);
		request_line >> method >> path >> http_version;
	}

	// Parse headers
	while (std::getline(stream, line) && line != "\r")
	{
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);

		std::size_t colon = line.find(": ");
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 2);
			headers[key] = value;
		}
	}
}

const std::string &requestParser::getMethod() const
{
	return method;
}

const std::string &requestParser::getPath() const
{
	return path;
}

const std::string &requestParser::getHttpVersion() const
{
	return http_version;
}

const std::map<std::string, std::string> &requestParser::getHeaders() const
{
	return headers;
}
