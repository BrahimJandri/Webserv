/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user <user@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/26 12:58:40 by user              #+#    #+#             */
/*   Updated: 2025/06/28 10:23:10 by user             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResponseBuilder.hpp"
#include <sys/stat.h>
#include <fstream>
#include <cstdio>

std::string ResponseBuilder::getFileExtension(const std::string& filePath) {
	std::string extension = "";
	size_t pos;

	pos = filePath.rfind('.');
	if (pos != filePath.npos) {
		extension = filePath.substr(pos);
	}
	return (extension);
}

bool ResponseBuilder::fileExists(const std::string& filePath) {
	struct stat buffer;
	
	return (stat(filePath.c_str(), &buffer) == 0);
}

std::string ResponseBuilder::getContentType(const std::string& filePath) {
	static std::map<std::string, std::string> mimeTypes;
	std::string unknown = "application/octet-stream";

	if (mimeTypes.empty()) {
		mimeTypes[".html"] = "text/html";
		mimeTypes[".css"] = "text/css";
		mimeTypes[".js"] = "application/javascript";
		mimeTypes[".txt"] = "text/plain";
		mimeTypes[".jpg"] = "image/jpeg";
		mimeTypes[".jpeg"] = "image/jpeg";
		mimeTypes[".png"] = "image/png";
		mimeTypes[".gif"] = "image/gif";
		mimeTypes[".ico"] = "image/x-icon";
	}
	std::string ext = getFileExtension(filePath);
	auto iter = mimeTypes.find(ext);
	if (iter != mimeTypes.end())
		return (iter->second);
	else
		return (unknown);
}

Response ResponseBuilder::buildErrorResponse(int statusCode, const std::string& statusMessage) {
	Response response;
	response.setStatus(statusCode, statusMessage);
	response.addHeader("Content-Type", "text/html");
	
	// Create simple HTML error page
	std::string htmlBody = "<!DOCTYPE html>\n<html>\n<head><title>" + 
		std::to_string(statusCode) + " " + statusMessage + "</title></head>\n<body>\n<h1>" + 
		std::to_string(statusCode) + " " + statusMessage + "</h1>\n<p>The requested resource encountered an error.</p>\n</body>\n</html>";
	
	response.setBody(htmlBody);
	return response;
}

Response ResponseBuilder::buildFileResponse(const std::string& filePath) {
	if (!fileExists(filePath))
		return (buildErrorResponse(404, "Not Found"));
		
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
		return (buildErrorResponse(500, "Internal Server Error"));
		
	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	
	std::string content(fileSize, '\0');
	file.read(&content[0], fileSize);
	file.close();
	
	Response response;
	response.setStatus(200, "OK");
	response.addHeader("Content-Type", getContentType(filePath));
	response.setBody(content);
	
	return (response);
}

Response ResponseBuilder::buildGetResponse(const requestParser& request, const std::string& docRoot) {
	std::string requestPath = request.getPath();
	if (requestPath.compare("/") == 0)
		requestPath = "/index.html";
	std::string fullPath = docRoot + requestPath;
	return buildFileResponse(fullPath);
}

Response ResponseBuilder::buildDeleteResponse(const requestParser& request, const std::string& docRoot) {
	std::string requestPath = request.getPath();
	
	if (requestPath.compare("/") == 0)
		return buildErrorResponse(403, "Forbidden");
		
	std::string fullPath = docRoot + requestPath;
	
	if (!fileExists(fullPath))
		return buildErrorResponse(404, "Not Found");
	
	if (remove(fullPath.c_str()) == 0) {
		Response response;
		response.setStatus(200, "OK");
		response.addHeader("Content-Type", "text/html");
		
		std::string htmlBody = "<!DOCTYPE html>\n<html>\n<head><title>File Deleted</title></head>\n<body>\n<h1>File Successfully Deleted</h1>\n<p>The file " + requestPath + " has been removed.</p>\n</body>\n</html>";
		response.setBody(htmlBody);
		return response;
	}
	else
		return buildErrorResponse(500, "Internal Server Error");
}
