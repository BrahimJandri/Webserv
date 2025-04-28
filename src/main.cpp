#include "Config/ConfigParser.hpp"
#include "HTTP/Request.hpp"
#include "Utils/Logger.hpp"
#include "Utils/AnsiColor.hpp"
#include "Server/Server.hpp"
#include <iostream>

int main(int ac, char **av)
{
	if (ac == 2)
	{
		Utils::log("Starting Webserv with configuration: " + std::string(av[1]), AnsiColor::GREEN);
		ConfigParser parser(av[1]);
		requestParser req; // hada class fih parsed request

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
			int fd = create_server_socket(servers[i].host, servers[i].port);
			if (fd != -1)
				handle_requests(fd, req);
			else
				Utils::logError("Failed to create socket for server " + Utils::intToString(i));
		}
	}
	else
	{
		std::cerr << AnsiColor::RED << "Usage: " << av[0] << " <config_file.conf>" << AnsiColor::RESET << std::endl;
		return 1;
	}
	return 0;
}
