#include "Server.hpp"
#include "../Config/ConfigParser.hpp"
#include "../Utils/Logger.hpp" // Make sure Logger.hpp defines LOG_ERROR, LOG_INFO etc. if used
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>   // For open
#include <sys/stat.h> // For stat, fstat

// Helper function to convert size_t to std::string (C++98 compatible)
std::string to_string_c98(size_t val)
{
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

// Helper function to send an HTTP response (status line and headers only)
void send_http_headers(int client_fd, const std::string& status_line,
                       const std::string& content_type, size_t content_length,
                       const std::string& connection_header = "close")
{
    std::ostringstream oss;
    oss << status_line << "\r\n"
        << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << content_length << "\r\n"
        << "Connection: " << connection_header << "\r\n\r\n"; // End of headers

    std::string header_response = oss.str();
    write(client_fd, header_response.c_str(), header_response.length());
}

// Helper function to send an error response
void send_error_response(int client_fd, int status_code, const std::string& message)
{
    std::string status_text;
    std::string error_page_path;
    if (status_code == 404) {
        status_text = "Not Found";
        error_page_path = "Error_pages/404.html";
    } else if (status_code == 500) {
        status_text = "Internal Server Error";
        error_page_path = "Error_pages/500.html";
    } else {
        status_text = "Unknown Error";
        error_page_path = "Error_pages/500.html"; // Fallback
    }

    std::string response_body = message; // Default body if error page not found

    int error_fd = open(error_page_path.c_str(), O_RDONLY);
    if (error_fd >= 0) {
        struct stat st;
        fstat(error_fd, &st);
        response_body.resize(st.st_size);
        read(error_fd, &response_body[0], st.st_size);
        close(error_fd);
    } else {
        // If error page itself not found, use a simple HTML message
        response_body = "<!DOCTYPE html><html><head><title>Error " + to_string_c98(status_code) + "</title></head><body><h1>Error " + to_string_c98(status_code) + ": " + status_text + "</h1><p>" + message + "</p></body></html>";
    }

    send_http_headers(client_fd, "HTTP/1.1 " + to_string_c98(status_code) + " " + status_text,
                      "text/html", response_body.length());
    write(client_fd, response_body.c_str(), response_body.length());
}

// Helper function to serve a static file
void serve_static_file(int client_fd, const std::string& file_path, const std::string& content_type)
{
    int file_fd = open(file_path.c_str(), O_RDONLY);
    if (file_fd < 0)
    {
        // File not found or inaccessible, send 404
        perror(("open " + file_path).c_str());
        send_error_response(client_fd, 404, "The requested resource '" + file_path + "' was not found.");
        return;
    }

    struct stat st;
    fstat(file_fd, &st);
    size_t file_size = st.st_size;

    send_http_headers(client_fd, "HTTP/1.1 200 OK", content_type, file_size);

    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0)
    {
        write(client_fd, buffer, bytes_read);
    }

    close(file_fd);
}

// Your main request handling loop
void handle_requests(int server_fd, requestParser &req)
{
    while (true)
    {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0)
        {
            perror("accept");
            // LOG_ERROR("accept failed"); // If you have a Logger
            continue;
        }

        char buffer[4096];
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0)
        {
            // Read error or client disconnected
            close(client_fd);
            continue;
        }

        std::string raw_request(buffer);
        req.parseRequest(raw_request); // Assuming parseRequest handles all parsing (method, path, headers, body)

        std::cout << "== New HTTP Request ==\n";
        std::cout << "Method: " << req.getMethod() << "\n";
        std::cout << "Path: " << req.getPath() << "\n";
        std::cout << "HTTP Version: " << req.getHttpVersion() << std::endl;
        std::cout << "Headers:\n";
        std::map<std::string, std::string>::const_iterator it;
        for (it = req.getHeaders().begin(); it != req.getHeaders().end(); ++it)
        {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
        std::cout << "Body:\n" << req.getBody() << std::endl; // Assuming getBody() exists and returns the request body

        // --- Request Routing Logic ---
        if (req.getMethod() == "GET")
        {
            if (req.getPath() == "/")
            {
                // Serve your web tester page
                serve_static_file(client_fd, "www/index.html", "text/html");
            }
            else if (req.getPath() == "/images/agadir.jpg")
            {
                // Serve the image
                serve_static_file(client_fd, "images/agadir.jpg", "image/jpeg");
            }
            // You can add more GET routes here, e.g., for CSS, JS files if they are in separate paths
            // else if (req.getPath() == "/css/style.css") {
            //     serve_static_file(client_fd, "www/css/style.css", "text/css");
            // }
            else
            {
                // No specific handler for this GET path
                send_error_response(client_fd, 404, "Resource not found for GET request.");
            }
        }
        else if (req.getMethod() == "POST")
        {
            if (req.getPath() == "/data")
            {
                // Example POST handler: just echo back the received body
                std::string response_body = "Received POST data:\n" + req.getBody();
                send_http_headers(client_fd, "HTTP/1.1 200 OK", "text/plain", response_body.length());
                write(client_fd, response_body.c_str(), response_body.length());
                std::cout << "Handled POST request to /data with body:\n" << req.getBody() << std::endl;
            }
            else
            {
                send_error_response(client_fd, 405, "Method Not Allowed for this path.");
            }
        }
        else if (req.getMethod() == "DELETE")
        {
            if (req.getPath().rfind("/data/", 0) == 0) // Checks if path starts with "/data/"
            {
                // Example DELETE handler: parse ID and send a success message
                std::string resource_id = req.getPath().substr(req.getPath().find_last_of('/') + 1);
                std::string response_body = "Successfully processed DELETE request for resource ID: " + resource_id;
                send_http_headers(client_fd, "HTTP/1.1 200 OK", "text/plain", response_body.length());
                write(client_fd, response_body.c_str(), response_body.length());
                std::cout << "Handled DELETE request for resource ID: " << resource_id << std::endl;
            }
            else
            {
                send_error_response(client_fd, 405, "Method Not Allowed for this path.");
            }
        }
        else
        {
            // Method not supported or implemented
            send_error_response(client_fd, 405, "Method Not Allowed.");
        }

        close(client_fd);
    }
}

// Your create_server_socket and to_string_c98 functions remain the same
int create_server_socket(const std::string &host, int port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1)
    {
        perror("listen");
        close(server_fd);
        return -1;
    }

    std::cout << "Listening on " << host << ":" << port << std::endl;
    return server_fd;
}

// to_string_c98 is already defined above the handle_requests function now.
// std::string to_string_c98(size_t val)
// {
//     std::ostringstream oss;
//     oss << val;
//     return oss.str();
// }