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

	if (!parser.parse())
	{
		Utils::logError("Failed to parse configuration file");
		return 1;
	}

	const std::vector<ServerConfig> &servers = parser.getServers();
	Utils::log("Found " + Utils::intToString(servers.size()) + " server configurations", AnsiColor::CYAN);

	for (size_t i = 0; i < servers.size(); ++i)
	{
		Utils::log("Setting up server on " + servers[i].host + ":" + Utils::intToString(servers[i].port), AnsiColor::YELLOW);
		
		// Create a Server instance and use the proper epoll-based approach
		Server server;
		server.start(servers[i].host, servers[i].port);
		// This will use the Client class with proper Content-Length handling
	}
	return 0;
}

int main(int ac, char **av)
{
	if (ac == 2)
		return start_server(av[1]);
	else if (ac == 1)
		return start_server("../conf/default.conf");
	else
	{
		std::cerr << AnsiColor::RED << "Usage: " << av[0] << " <config_file.conf>" << AnsiColor::RESET << std::endl;
		return 1;
	}
}
