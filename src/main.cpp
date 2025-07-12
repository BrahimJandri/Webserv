#include "Config/ConfigParser.hpp"
#include "HTTP/Request.hpp"
#include "Utils/Logger.hpp"
#include "Utils/AnsiColor.hpp"
#include "Server/Server.hpp"
#include <iostream>
#include <fcntl.h>

int start_server(const std::string &config_path)
{
	Utils::log("Starting Webserv with configuration: " + config_path, AnsiColor::GREEN);
	ConfigParser parser(config_path);

	parser.parse();
	size_t serverCount = parser.getServerCount();
	Utils::log("Found " + Utils::intToString(serverCount) + " server configurations", AnsiColor::CYAN);

	for (size_t i = 0; i < serverCount; ++i)
	{
		const ServerConfig &servers = parser.getServerConfig(i);
		ConfigValue host = servers.getDirective("host");
		ConfigValue port = servers.getDirective("port");
		Utils::log("Setting up server on " + host.asString() + ":" + Utils::intToString(port.asInt()), AnsiColor::YELLOW);

		int server_fd = create_server_socket(host.asString(), port.asInt());
		if (server_fd == -1)
		{
			std::cerr << "Failed to create server socket." << std::endl;
			exit(EXIT_FAILURE);
		}
		// Create a Server instance and use the proper epoll-based approach
		// This will use the Client class with proper Content-Length handling
	}
	// handleConnections(server_fd);
	exit(EXIT_SUCCESS);
}

int main(int ac, char **av)
{
	if (ac == 2)
		return start_server(av[1]);
	else if (ac == 1)
		return start_server("./conf/default.conf");
	else
	{
		std::cerr << AnsiColor::RED << "Usage: " << av[0] << " <config_file.conf>" << AnsiColor::RESET << std::endl;
		return 1;
	}
}
