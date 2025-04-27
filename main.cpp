#include "./config/parserHeader.hpp"
#include "./config/requestParser.hpp"
#include <iostream>

int main(int ac, char **av)
{
    if (ac == 2)
    {
        ConfigParser parser(av[1]);
        requestParser req;

        if (!parser.parse())
            return 1;

        const std::vector<ServerConfig> &servers = parser.getServers();

        for (size_t i = 0; i < servers.size(); ++i)
        {
            int fd = create_server_socket(servers[i].host, servers[i].port);
            if (fd != -1)
                handle_requests(fd, req);
        }
    }
    else
    {
        std::cerr << "Usage: " << av[0] << " <config_file.conf>" << std::endl;
        return 1;
    }
    return 0;
}
