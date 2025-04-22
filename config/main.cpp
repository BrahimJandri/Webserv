#include "ConfigParser.hpp"
#include <iostream>


int main(int ac, char **av)
{
    if (ac == 2)
    {
        ConfigParser parser(av[1]);
        if (!parser.parse())
            return 1;

        const std::vector<ServerConfig> &servers = parser.getServers();
        // for (size_t i = 0; i < servers.size(); ++i)
        // {
        //     std::cout << "Server " << i << " on " << servers[i].host << ":" << servers[i].port << std::endl;
        //     std::cout << "  Server name: " << servers[i].server_name << std::endl;
        //     for (size_t j = 0; j < servers[i].locations.size(); ++j)
        //     {
        //         std::cout << "    Location " << servers[i].locations[j].path << std::endl;
        //         std::cout << "      Root: " << servers[i].locations[j].root << std::endl;
        //         std::cout << "      Index: " << servers[i].locations[j].index << std::endl;
        //     }
        // }
        
        for (size_t i = 0; i < servers.size(); ++i)
        {
            int fd = create_server_socket(servers[i].host, servers[i].port);
            if (fd != -1)
                handle_requests(fd); // This blocks, so maybe do one for now
        }
    }
    else
    {
        std::cerr << "Usage: " << av[0] << " <config_file.conf>" << std::endl;
        return 1;
    }
    return 0;
}
