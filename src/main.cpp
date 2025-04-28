#include "Config/ConfigParser.hpp"
#include "HTTP/Request.hpp"
#include "Utils/Logger.hpp"
#include "Utils/AnsiColor.hpp"
#include "Server/Server.hpp"
#include <iostream>
#include <vector>

int main(int ac, char **av)
{
	if (ac == 2)
	{
		Utils::log("Starting Webserv with configuration: " + std::string(av[1]), AnsiColor::GREEN);
		ConfigParser parser(av[1]);

		if (!parser.parse())
		{
			Utils::logError("Failed to parse configuration file");
			return 1;
		}

		const std::vector<ServerConfig> &servers = parser.getServers();
		Utils::log("Found " + Utils::intToString(servers.size()) + " server configurations", AnsiColor::CYAN);

		std::vector<Server*> server_instances;

		for (size_t i = 0; i < servers.size(); ++i)
		{
			Utils::log("Setting up server on " + servers[i].host + ":" + Utils::intToString(servers[i].port), AnsiColor::YELLOW);
			
			// Create a new server instance using our multiplexing Server class
			Server* server = new Server(servers[i].host, servers[i].port);
			
			if (server->setup())
			{
				server_instances.push_back(server);
				Utils::log("Server ready on " + servers[i].host + ":" + Utils::intToString(servers[i].port), AnsiColor::GREEN);
			}
			else
			{
				Utils::logError("Failed to setup server on " + servers[i].host + ":" + Utils::intToString(servers[i].port));
				delete server;
			}
		}
		
		// Run all server instances that were successfully set up
		if (!server_instances.empty())
		{
			// In a production server, you would fork or use threads for each server
			// For simplicity, we'll just run the first one
			Utils::log("Starting server on " + server_instances[0]->getHost() + ":" + 
			           Utils::intToString(server_instances[0]->getPort()), AnsiColor::CYAN);
			
			server_instances[0]->run();
			
			// This code won't be reached until the server stops
			Utils::log("Server stopped", AnsiColor::YELLOW);
		}
		else
		{
			Utils::logError("No servers could be started");
			return 1;
		}
		
		// Clean up
		for (size_t i = 0; i < server_instances.size(); ++i)
		{
			delete server_instances[i];
		}
	}
	else
	{
		std::cerr << AnsiColor::RED << "Usage: " << av[0] << " <config_file.conf>" << AnsiColor::RESET << std::endl;
		return 1;
	}
	
	return 0;
}
