#include "Config/ConfigParser.hpp"
#include "HTTP/Request.hpp"
#include "Utils/Logger.hpp"
#include "Utils/AnsiColor.hpp"
#include "Server/Server.hpp"
#include <iostream>
#include <fcntl.h>

int main(int ac, char **av)
{
	if (ac == 2)
	{
		Utils::log("Starting Webserv with configuration: " + std::string(av[1]), AnsiColor::GREEN);
		requestParser req; // hada class fih parsed request
		// ConfigParser parser(av[1]);

		std::string configFile = av[1];
		ConfigParser parser(configFile);
		
		parser.parse();

		
		
		if (!parser.getServerCount())
		{
			Utils::logError("No server configurations found in the provided config file.");
			return 1;
		}
		
		// Utils::log("Found " + Utils::intToString(servers.size()) + " server configurations", AnsiColor::CYAN);
		
		for (size_t i = 0; i < parser.getServerCount(); ++i)
		{
			const ServerConfig& servers = parser.getServerConfig(i);
			ConfigValue host = servers.getDirective("host");
			ConfigValue port = servers.getDirective("port");

			Utils::log("Setting up server on " + host.asString() + ":" + Utils::intToString(port.asInt()), AnsiColor::YELLOW);

			int fd = create_server_socket(host.asString(), port.asInt());
			if (fd != -1)
			{
				// Make the socket non-blocking
				int flags = fcntl(fd, F_GETFL, 0);
				fcntl(fd, F_SETFL, flags | O_NONBLOCK);
				
				Server server;
				server.handleConnections(fd);  // This will start the event loop
			}
			else
			{
				Utils::logError("Failed to create socket for server " + Utils::intToString(i));
			}
		}
	}
	else
	{
		std::cerr << AnsiColor::RED << "Usage: " << av[0] << " <config_file.conf>" << AnsiColor::RESET << std::endl;
		return 1;
	}
	return 0;
}

