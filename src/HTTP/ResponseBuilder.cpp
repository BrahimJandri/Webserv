/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user <user@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/26 12:58:40 by user              #+#    #+#             */
/*   Updated: 2025/07/03 10:50:59 by user             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResponseBuilder.hpp"
#include <sys/stat.h>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <sstream>
#include <cstdlib>

// Helper function to URL decode strings
std::string urlDecode(const std::string& str) {
	std::string result;
	for (size_t i = 0; i < str.length(); ++i) {
		if (str[i] == '%' && i + 2 < str.length()) {
			// Convert hex to char
			std::string hex = str.substr(i + 1, 2);
			char ch = static_cast<char>(strtol(hex.c_str(), NULL, 16));
			result += ch;
			i += 2;
		} else if (str[i] == '+') {
			result += ' ';
		} else {
			result += str[i];
		}
	}
	return result;
}

std::string ResponseBuilder::getFileExtension(const std::string &filePath)
{
	std::string extension = "";
	size_t pos;

	pos = filePath.rfind('.');
	if (pos != filePath.npos)
	{
		extension = filePath.substr(pos);
	}
	return (extension);
}

bool ResponseBuilder::fileExists(const std::string &filePath)
{
	struct stat buffer;

	return (stat(filePath.c_str(), &buffer) == 0);
}

std::string ResponseBuilder::getContentType(const std::string &filePath)
{
	static std::map<std::string, std::string> mimeTypes;
	std::string unknown = "application/octet-stream";

	if (mimeTypes.empty())
	{
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
	std::map<std::string, std::string>::iterator iter = mimeTypes.find(ext);
	if (iter != mimeTypes.end())
		return (iter->second);
	else
		return (unknown);
}

Response ResponseBuilder::buildErrorResponse(int statusCode, const std::string &statusMessage)
{
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

Response ResponseBuilder::buildFileResponse(const std::string &filePath)
{
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

Response ResponseBuilder::buildGetResponse(const requestParser &request, const std::string &docRoot)
{
	std::string requestPath = request.getPath();
	if (requestPath.compare("/") == 0)
		requestPath = "/index.html";
	std::string fullPath = docRoot + requestPath;
	return buildFileResponse(fullPath);
}

Response ResponseBuilder::buildPostResponse(const requestParser &request, const std::string &docRoot)
{
	std::string requestPath = request.getPath();
	std::string requestBody = request.getBody();

	std::map<std::string, std::string> headers = request.getHeaders();
	std::string contentType = "";
	for (std::map<std::string, std::string>::iterator iter = headers.begin(); iter != headers.end(); ++iter)
	{
		std::string keyword = iter->first;
		std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
		if (keyword == "content-type")
		{
			contentType = iter->second;
			break;
		}
	}
	if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
	{
		// Parse form data: name=value&email=value&...
		std::map<std::string, std::string> formFields;
		std::string body = requestBody;
		
		// Split by '&' to get individual fields
		size_t start = 0;
		size_t end = 0;
		
		while ((end = body.find('&', start)) != std::string::npos) {
			std::string pair = body.substr(start, end - start);
			
			// Split by '=' to get name and value
			size_t equalPos = pair.find('=');
			if (equalPos != std::string::npos) {
				std::string key = urlDecode(pair.substr(0, equalPos));
				std::string value = urlDecode(pair.substr(equalPos + 1));
				formFields[key] = value;
			}
			start = end + 1;
		}
		
		// Handle the last field (after the last &)
		if (start < body.length()) {
			std::string pair = body.substr(start);
			size_t equalPos = pair.find('=');
			if (equalPos != std::string::npos) {
				std::string key = urlDecode(pair.substr(0, equalPos));
				std::string value = urlDecode(pair.substr(equalPos + 1));
				formFields[key] = value;
			}
		}
		
		// Save form data to a file
		std::string saveDir = docRoot + "/data";
		std::string fileName = "formData.txt";
		std::string fullPath = saveDir + "/" + fileName;
		
		std::ofstream file(fullPath.c_str());
		if (!file.is_open())
			return (buildErrorResponse(500, "Internal Server Error"));
		
		file << "Form Submission Data:\n";
		file << "Path: " << requestPath << "\n\n";
		
		for (std::map<std::string, std::string>::iterator iter = formFields.begin(); iter != formFields.end(); ++iter) {
			file << iter->first << ": " << iter->second << "\n";
		}
		file.close();
		
		// Create success response
		Response response;
		response.setStatus(201, "Created");
		response.addHeader("Content-Type", "text/html");
		
		std::string htmlBody = "<!DOCTYPE html>\n<html>\n<head><title>Form Submitted</title></head>\n<body>\n"
			"<h1>Form Successfully Submitted!</h1>\n"
			"<p>Your form data has been saved.</p>\n"
			"<h2>Submitted Data:</h2>\n<ul>\n";
		
		for (std::map<std::string, std::string>::iterator iter = formFields.begin(); iter != formFields.end(); ++iter) {
			htmlBody += "<li><strong>" + iter->first + ":</strong> " + iter->second + "</li>\n";
		}
		
		htmlBody += "</ul>\n<a href=\"/\">Back to Home</a>\n</body>\n</html>";
		response.setBody(htmlBody);
		
		return response;
	}
	else if (contentType.find("application/json") != std::string::npos)
	{
		std::string saveDir = docRoot + "/data";
		std::string fileName = "jsonData.json";
		std::string fullPath = saveDir + "/" + fileName;

		std::ofstream file(fullPath.c_str());
		if (!file.is_open())
			return (buildErrorResponse(500, "Internal Server Error"));
		file << requestBody;
		file.close();
		
		// Create success response directly
		Response response;
		response.setStatus(201, "Created");
		response.addHeader("Content-Type", "application/json");
		
		std::string jsonResponse = "{\"status\": \"success\", \"message\": \"JSON data saved\", \"filename\": \"" + fileName + "\"}";
		response.setBody(jsonResponse);
		
		return response;
	}
	else
	{
		// Default case - treat as plain text or unknown data
		std::string saveDir = docRoot + "/data";
		std::string fileName = "postData.txt";
		std::string fullPath = saveDir + "/" + fileName;
		
		std::ofstream file(fullPath.c_str());
		if (!file.is_open())
			return (buildErrorResponse(500, "Internal Server Error"));
		
		file << "POST Data Submission:\n";
		file << "Path: " << requestPath << "\n";
		file << "Content-Type: " << (contentType.empty() ? "Unknown" : contentType) << "\n\n";
		file << "Raw Body:\n" << requestBody;
		file.close();
		
		// Create success response
		Response response;
		response.setStatus(201, "Created");
		response.addHeader("Content-Type", "text/html");
		
		std::string htmlBody = "<!DOCTYPE html>\n<html>\n<head><title>Data Received</title></head>\n<body>\n"
			"<h1>Data Successfully Received!</h1>\n"
			"<p>Your data has been saved as plain text.</p>\n"
			"<p><strong>Content-Type:</strong> " + (contentType.empty() ? "Unknown" : contentType) + "</p>\n"
			"<a href=\"/\">Back to Home</a>\n</body>\n</html>";
		
		response.setBody(htmlBody);
		return response;
	}
}

Response ResponseBuilder::buildDeleteResponse(const requestParser &request, const std::string &docRoot)
{
	std::string requestPath = request.getPath();

	if (requestPath.compare("/") == 0)
		return buildErrorResponse(403, "Forbidden");

	std::string fullPath = docRoot + requestPath;

	if (!fileExists(fullPath))
		return buildErrorResponse(404, "Not Found");

	if (remove(fullPath.c_str()) == 0)
	{
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
