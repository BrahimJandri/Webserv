#pragma once

#include <string>
#include <map>
#include <sstream>

class requestParser	{
	private:
	std::string method;
	std::string path;
	std::string http_version;
	std::map<std::string, std::string> headers;

	public:
	requestParser();
	~requestParser();

	void parseRequest(const std::string &raw_request);

	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getHttpVersion() const;
	const std::map<std::string, std::string> &getHeaders() const;
};

