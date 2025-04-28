#pragma once

#include <string>
#include <map>

class Response {
private:
	int statusCode;
	std::string statusMessage;
	std::map<std::string, std::string> headers;
	std::string body;

	public:
	Response();
	~Response();

	void setStatus(int code, const std::string& message);
	void addHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& content);

	std::string toString() const;
};
