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
    ConfigParser parser;
    parser.parseFile(config_path);

    size_t serverCount = parser.getServerCount();
    Utils::log("Found " + Utils::intToString(serverCount) + " server configurations", AnsiColor::CYAN);

    Server server;
    server.setupServers(parser);      // 👈 create sockets + register to epoll
    server.handleConnections();       // 👈 single loop to handle all

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
