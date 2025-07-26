#include "Response.hpp"

// Constructor
Response::Response() : statusCode(200), statusMessage("OK"), _httpVersion("HTTP/1.1")
{
    addHeader("Server", "Webserv/1.0");
    addHeader("Connection", "close");
}
// Destructor
Response::~Response()
{
    // No dynamic memory allocated directly within Response object that needs explicit deletion,
    // as std::string and std::map handle their own memory.
}

void Response::setStatus(int code, const std::string &message)
{
    statusCode = code;
    statusMessage = message;
}

void Response::addHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

void Response::setBody(const std::string &body)
{
    _body = body;
}

void Response::setHttpVersion(const std::string &version)
{
    _httpVersion = version;
}

// Getters
int Response::getStatusCode() const
{
    return statusCode;
}

const std::string &Response::getStatusText() const
{
    return statusMessage;
}

const std::map<std::string, std::string> &Response::getHeaders() const
{
    return _headers;
}

const std::string &Response::getBody() const
{
    return _body;
}

const std::string &Response::getHttpVersion() const
{
    return _httpVersion;
}

// toString method to assemble the full HTTP response
std::string Response::toString() const
{
    std::ostringstream oss;

    // Status Line: HTTP-Version Status-Code Reason-Phrase CR LF
    oss << _httpVersion << " " << statusCode << " " << statusMessage << "\r\n";

    // Headers: Header-Name: Header-Value CR LF
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
    {
        oss << it->first << ": " << it->second << "\r\n";
    }

    // Always add Content-Length if a body exists and header isn't already set
    // (This ensures compliance, though CGIHandler explicitly adds it)
    if (_body.length() > 0 && _headers.find("Content-Length") == _headers.end())
    {
        oss << "Content-Length: " << to_string_c98(_body.length()) << "\r\n";
    }

    // End of headers: CR LF
    oss << "\r\n";

    // Body
    oss << _body;

    return oss.str();
}

// Helper function to URL decode strings
std::string urlDecode(const std::string &str)
{
    std::string result;
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '%' && i + 2 < str.length())
        {
            // Convert hex to char
            std::string hex = str.substr(i + 1, 2);
            char ch = static_cast<char>(strtol(hex.c_str(), NULL, 16));
            result += ch;
            i += 2;
        }
        else if (str[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += str[i];
        }
    }
    return result;
}

std::string Response::getFileExtension(const std::string &filePath)
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

bool Response::fileExists(const std::string &filePath)
{
    struct stat buffer;

    return (stat(filePath.c_str(), &buffer) == 0);
}

std::string Response::getContentType(const std::string &filePath)
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

Response Response::buildFileResponse(const std::string &filePath, const ConfigParser::ServerConfig &serverConfig, int client_fd)
{
    if (!fileExists(filePath))
    {
        std::cerr << "ERROR: File not found: " << filePath << std::endl;
        send_error_response(client_fd, 404, "Not Found", serverConfig);
        return Response();
    }

    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "ERROR: Could not open file: " << filePath << std::endl;
        send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
        return Response();
    }

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

Response Response::buildAutoindexResponse(const std::string &htmlContent)
{
    Response response;

    response.setStatus(200, "OK");
    std::stringstream headers;
    headers << "HTTP/1.1 200 OK\r\n";
    headers << "Content-Type: text/html\r\n";
    headers << "Content-Length: " << htmlContent.size() << "\r\n";
    headers << "Connection: close\r\n";
    headers << "\r\n";

    response.addHeader("Content-Type", "text/html");
    response.addHeader("charset", "UTF-8");
    response.addHeader("Content-Length", to_string_c98(htmlContent.size()));
    response.addHeader("Connection", "close");
    response.setBody(htmlContent);

    return response;
}

Response Response::buildGetResponse(const requestParser &request, const std::string &docRoot, const bool autoindex, int client_fd, const ConfigParser::ServerConfig &serverConfig)
{
    if (docRoot.empty())
    {
        send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
        return Response();
    }

    std::string requestPath = request.getPath();
    std::string fullPath = docRoot;

    if (!fileExists(fullPath))
    {
        send_error_response(client_fd, 404, "Not Found", serverConfig);
        return Response();
    }

    if (isDirectory(fullPath))
    {
        if (fullPath[fullPath.length() - 1] != '/')
            fullPath += '/';

        if (!serverConfig.locations.empty() && !serverConfig.locations[0].index.empty())
        {
            std::string indexPath = fullPath + serverConfig.locations[0].index[0];
            if (fileExists(indexPath))
            {
                fullPath = indexPath;
            }
            else if (autoindex)
            {
                std::string autoindexPage = generate_autoindex(fullPath, requestPath);
                return buildAutoindexResponse(autoindexPage);
            }
            else
            {
                send_error_response(client_fd, 403, "Forbidden", serverConfig);
                return Response();
            }
        }
        else
        {
            send_error_response(client_fd, 403, "Forbidden", serverConfig);
            return Response();
        }
    }

    if (access(fullPath.c_str(), R_OK) != 0)
    {
        send_error_response(client_fd, 403, "Forbidden", serverConfig);
        return Response();
    }

    return buildFileResponse(fullPath, serverConfig, client_fd);
}

Response Response::buildPostResponse(const requestParser &request, const std::string &docRoot, int client_fd, const ConfigParser::ServerConfig &serverConfig)
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

        while ((end = body.find('&', start)) != std::string::npos)
        {
            std::string pair = body.substr(start, end - start);

            // Split by '=' to get name and value
            size_t equalPos = pair.find('=');
            if (equalPos != std::string::npos)
            {
                std::string key = urlDecode(pair.substr(0, equalPos));
                std::string value = urlDecode(pair.substr(equalPos + 1));
                formFields[key] = value;
            }
            start = end + 1;
        }

        // Handle the last field (after the last &)
        if (start < body.length())
        {
            std::string pair = body.substr(start);
            size_t equalPos = pair.find('=');
            if (equalPos != std::string::npos)
            {
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
        {
            send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
            return Response();
        }

        file << "Form Submission Data:\n";
        file << "Path: " << requestPath << "\n\n";

        for (std::map<std::string, std::string>::iterator iter = formFields.begin(); iter != formFields.end(); ++iter)
        {
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

        for (std::map<std::string, std::string>::iterator iter = formFields.begin(); iter != formFields.end(); ++iter)
        {
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
        {
            send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
            return Response();
        }

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
    else if (contentType.find("multipart/form-data") != std::string::npos)
    {
        // Extract boundary from Content-Type header
        std::string boundary;
        size_t boundaryPos = contentType.find("boundary=");
        if (boundaryPos != std::string::npos)
        {
            boundary = "--" + contentType.substr(boundaryPos + 9);
            // Remove any trailing whitespace or semicolons
            size_t endPos = boundary.find_first_of(" ;\r\n");
            if (endPos != std::string::npos)
            {
                boundary = boundary.substr(0, endPos);
            }
        }

        if (boundary.empty())
        {
            send_error_response(client_fd, 400, "Bad Request: Missing boundary in multipart/form-data", serverConfig);
            return Response();
        }

        // Variables to store parsed data
        std::vector<std::string> uploadedFiles;
        std::map<std::string, std::string> formFields;

        // Split body into parts using boundary
        std::string closingBoundary = boundary + "--";
        size_t pos = 0;

        while (pos < requestBody.length())
        {
            // Find next boundary
            size_t boundaryStart = requestBody.find(boundary, pos);
            if (boundaryStart == std::string::npos)
            {
                break;
            }

            // Move past the boundary
            pos = boundaryStart + boundary.length();

            // Skip CRLF after boundary
            if (pos + 1 < requestBody.length() &&
                requestBody[pos] == '\r' && requestBody[pos + 1] == '\n')
            {
                pos += 2;
            }

            // Find the next boundary to determine end of this part
            size_t nextBoundaryStart = requestBody.find(boundary, pos);
            if (nextBoundaryStart == std::string::npos)
            {
                break;
            }

            // Extract the current part (between boundaries)
            std::string part = requestBody.substr(pos, nextBoundaryStart - pos);

            // Remove trailing CRLF before the next boundary
            if (part.length() >= 2 &&
                part[part.length() - 2] == '\r' && part[part.length() - 1] == '\n')
            {
                part = part.substr(0, part.length() - 2);
            }

            // Find the empty line that separates headers from content
            size_t headerEnd = part.find("\r\n\r\n");
            if (headerEnd == std::string::npos)
            {
                pos = nextBoundaryStart;
                continue;
            }

            // Extract headers and content
            std::string headers = part.substr(0, headerEnd);
            std::string content = part.substr(headerEnd + 4);

            // Parse Content-Disposition header to get name and filename
            std::string name, filename;

            if (headers.find("Content-Disposition") != std::string::npos)
            {
                // Find name parameter
                size_t namePos = headers.find("name=\"");
                if (namePos != std::string::npos)
                {
                    namePos += 6; // Skip 'name="'
                    size_t nameEnd = headers.find("\"", namePos);
                    if (nameEnd != std::string::npos)
                    {
                        name = headers.substr(namePos, nameEnd - namePos);
                    }
                }

                // Find filename parameter
                size_t filenamePos = headers.find("filename=\"");
                if (filenamePos != std::string::npos)
                {
                    filenamePos += 10; // Skip 'filename="'
                    size_t filenameEnd = headers.find("\"", filenamePos);
                    if (filenameEnd != std::string::npos)
                    {
                        filename = headers.substr(filenamePos, filenameEnd - filenamePos);
                    }
                }
            }

            // If it's a file upload (has filename), save the file
            if (!filename.empty())
            {
                std::string saveDir = docRoot;
                std::string fullPath = saveDir + "/" + filename;

                // Save file in binary mode
                std::ofstream file(fullPath.c_str(), std::ios::binary);
                if (file.is_open())
                {
                    file.write(content.c_str(), content.length());
                    file.close();
                    uploadedFiles.push_back(filename);
                }
            }
            // Otherwise, it's a regular form field
            else if (!name.empty())
            {
                formFields[name] = content;
            }

            // Move to next part
            pos = nextBoundaryStart;
        }

        // Create response showing uploaded files and form fields
        Response response;
        response.setStatus(201, "Created");
        response.addHeader("Content-Type", "text/html");

        std::string htmlBody = "<!DOCTYPE html>\n"
                               "<html>\n"
                               "<head>\n"
                               "  <meta charset='UTF-8'>\n"
                               "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
                               "  <title>Upload Success</title>\n"
                               "  <style>\n"
                               "    body {\n"
                               "      font-family: Arial, sans-serif;\n"
                               "      background-color: #f4f4f4;\n"
                               "      color: #333;\n"
                               "      text-align: center;\n"
                               "      padding: 50px;\n"
                               "    }\n"
                               "    h1 {\n"
                               "      color: #28a745;\n"
                               "      font-size: 2.5rem;\n"
                               "    }\n"
                               "    .message {\n"
                               "      margin-top: 20px;\n"
                               "      padding: 20px;\n"
                               "      background-color: #fff;\n"
                               "      border-radius: 10px;\n"
                               "      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);\n"
                               "      display: inline-block;\n"
                               "    }\n"
                               "  </style>\n"
                               "</head>\n"
                               "<body>\n"
                               "  <div class='message'>\n"
                               "    <h1>Upload Successful!</h1>\n"
                               "    <p>Your file has been uploaded.</p>\n"
                               "  </div>\n"
                               "</body>\n"
                               "</html>\n";

        if (!uploadedFiles.empty())
        {
            htmlBody += "<h2>Uploaded Files:</h2>\n<ul>\n";
            for (size_t i = 0; i < uploadedFiles.size(); ++i)
            {
                htmlBody += "<li>" + uploadedFiles[i] + "</li>\n";
            }
            htmlBody += "</ul>\n";
        }

        if (!formFields.empty())
        {
            htmlBody += "<h2>Form Fields:</h2>\n<ul>\n";
            for (std::map<std::string, std::string>::iterator iter = formFields.begin(); iter != formFields.end(); ++iter)
            {
                htmlBody += "<li><strong>" + iter->first + ":</strong> " + iter->second + "</li>\n";
            }
            htmlBody += "</ul>\n";
        }

        htmlBody += "<a href=\"/\">Back to Home</a>\n</body>\n</html>";
        response.setBody(htmlBody);
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
        {
            send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
            return Response();
        }
        file << "POST Data Submission:\n";
        file << "Path: " << requestPath << "\n";
        file << "Content-Type: " << (contentType.empty() ? "Unknown" : contentType) << "\n\n";
        file << "Raw Body:\n"
             << requestBody;
        file.close();

        // Create success response
        Response response;
        response.setStatus(201, "Created");
        response.addHeader("Content-Type", "text/html");

        std::string htmlBody = "<!DOCTYPE html>\n<html>\n<head><title>Data Received</title></head>\n<body>\n"
                               "<h1>Data Successfully Received!</h1>\n"
                               "<p>Your data has been saved as plain text.</p>\n"
                               "<p><strong>Content-Type:</strong> " +
                               (contentType.empty() ? "Unknown" : contentType) + "</p>\n"
                                                                                 "<a href=\"/\">Back to Home</a>\n</body>\n</html>";

        response.setBody(htmlBody);
        return response;
    }
}

bool deleteDirectory(const std::string &path)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return false;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        std::string fullPath = path + "/" + name;

        struct stat entryStat;
        if (stat(fullPath.c_str(), &entryStat) == 0)
        {
            if (S_ISDIR(entryStat.st_mode))
            {
                if (!deleteDirectory(fullPath))
                {
                    closedir(dir);
                    return false;
                }
            }
            else
            {
                if (remove(fullPath.c_str()) != 0)
                {
                    closedir(dir);
                    return false;
                }
            }
        }
    }
    closedir(dir);

    // Finally delete the now-empty directory
    return rmdir(path.c_str()) == 0;
}

Response Response::buildDeleteResponse(const requestParser &request, const std::string &docRoot, int client_fd, const ConfigParser::ServerConfig &serverConfig)
{
    std::string requestPath = request.getPath();
    std::string fullPath = docRoot;
    ;

    // Prevent dangerous deletions or deleting root/index
    if (requestPath.empty() || requestPath == "/" || requestPath.find("..") != std::string::npos)
    {
        std::cerr << "ERROR: Invalid delete request path: " << requestPath << std::endl;
        send_error_response(client_fd, 400, "Bad Request", serverConfig);
        return Response();
    }

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) != 0)
    {
        perror("stat failed");
        if (errno == ENOENT)
        {
            send_error_response(client_fd, 404, "Not Found", serverConfig);
            return Response();
        }
        send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
        return Response();
    }

    if (access(fullPath.c_str(), W_OK) != 0)
    {
        perror("access failed");
        send_error_response(client_fd, 403, "Forbidden", serverConfig);
        return Response();
    }

    if (S_ISDIR(pathStat.st_mode))
    {
        if (!deleteDirectory(fullPath))
        {
            std::cerr << "Failed to delete directory: " << fullPath << std::endl;
            send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
            return Response();
        }
    }
    else
    {
        if (remove(fullPath.c_str()) != 0)
        {
            perror("remove failed");
            send_error_response(client_fd, 500, "Internal Server Error", serverConfig);
            return Response();
        }
    }

    Response response;
    response.setStatus(200, "OK");
    response.addHeader("Content-Type", "text/html");

    std::string htmlBody = "<!DOCTYPE html>\n"
                           "<html lang=\"en\">\n"
                           "<head>\n"
                           "  <meta charset=\"UTF-8\">\n"
                           "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                           "  <title>Deleted</title>\n"
                           "  <script src=\"https://cdn.tailwindcss.com\"></script>\n"
                           "  <link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap\" rel=\"stylesheet\" />\n"
                           "  <style>\n"
                           "    body { font-family: 'Inter', sans-serif; background-color: #f3f4f6; color: #1f2937; }\n"
                           "  </style>\n"
                           "</head>\n"
                           "<body class=\"flex items-center justify-center min-h-screen\">\n"
                           "  <div class=\"bg-white p-8 rounded-xl shadow max-w-md w-full text-center\">\n"
                           "    <h1 class=\"text-3xl font-bold mb-4 text-green-600\">Deletion Successful</h1>\n"
                           "    <p class=\"mb-6 text-gray-700\">The resource <code class=\"bg-gray-100 px-2 py-1 rounded\">" +
                           requestPath +
                           "</code> has been deleted.</p>\n"
                           "    <a href=\"/\" class=\"inline-block bg-blue-600 text-white px-6 py-3 rounded-lg hover:bg-blue-700 transition\">Back to Home</a>\n"
                           "  </div>\n"
                           "</body>\n"
                           "</html>";

    response.setBody(htmlBody);
    return response;
}
