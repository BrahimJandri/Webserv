#include "Server.hpp"
#include "../Config/ConfigParser.hpp"
#include "../Utils/Logger.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
	addr.sin_port = htons(port);					// This function is used to convert the unsigned int from machine byte order to network byte order.
	addr.sin_addr.s_addr = inet_addr(host.c_str()); // It is used when we don't want to bind our socket to any particular IP and instead make it listen to all the available IPs.

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

std::string to_string_c98(size_t val)
{
	std::ostringstream oss;
	oss << val;
	return oss.str();
}

void handle_requests(int server_fd, requestParser &req)
{
	while (true)
	{
		int client_fd = accept(server_fd, NULL, NULL);
		if (client_fd < 0)
		{
			perror("accept");
			continue;
		}

		char buffer[4096];
		std::memset(buffer, 0, sizeof(buffer));
		ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
		if (bytes_read <= 0)
		{
			close(client_fd);
			continue;
		}

		std::string raw_request(buffer);
		req.parseRequest(raw_request);

		std::cout << "== New HTTP Request ==\n";
		std::cout << req.getMethod() << " " << req.getPath() << " " << req.getHttpVersion() << std::endl;
		std::map<std::string, std::string>::const_iterator it;
		for (it = req.getHeaders().begin(); it != req.getHeaders().end(); ++it)
		{
			std::cout << it->first << ": " << it->second << std::endl;
		}

		// std::string body = "<h1>Hello from Webserv</h1>";
		// std::string response =
		// 	"HTTP/ 200 OK\r\n"
		// 	"Content-Type: image/jpg\r\n";
		// write(client_fd, response.c_str(), response.length());
		// close(client_fd);
		{
			const char *image_path = "./images/agadir.jpg";

			// Open image file in binary mode
			int img_fd = open(image_path, O_RDONLY);
			if (img_fd < 0)
			{
				std::string error_response = "HTTP/1.1 404 Not Found\r\n\r\nImage not found";
				write(client_fd, error_response.c_str(), error_response.length());
				close(client_fd);
				return;
			}

			// Get image size
			struct stat st;
			fstat(img_fd, &st);
			size_t img_size = st.st_size;

			// Construct HTTP response headers
			std::ostringstream oss;
			oss << "HTTP/1.1 200 OK\r\n"
				<< "Content-Type: image/jpeg\r\n"
				<< "Content-Length: " << img_size << "\r\n"
				<< "Connection: close\r\n\r\n"; // End of headers

			std::string header = oss.str();
			write(client_fd, header.c_str(), header.length());

			// Send image data
			char buffer[4096];
			ssize_t bytes_read;
			while ((bytes_read = read(img_fd, buffer, sizeof(buffer))) > 0)
			{
				write(client_fd, buffer, bytes_read);
			}

			close(img_fd);
			close(client_fd);
		}
	}
}
