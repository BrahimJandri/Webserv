#include "webserv.hpp"
#include <iostream>

int main()
{
    std::vector<ServerConfig> servers = parse_config("config.conf");

    for (size_t i = 0; i < servers.size(); ++i)
    {
        std::cout << "🔹 Server " << i + 1 << ":\n";
        std::cout << "  Port: " << servers[i].port << "\n";
        std::cout << "  Name: " << servers[i].server_name << "\n";
        std::cout << "  Root: " << servers[i].root << "\n";

        for (size_t j = 0; j < servers[i].locations.size(); ++j)
        {
            std::cout << "    📁 Location " << servers[i].locations[j].path << "\n";
            std::cout << "      Method: " << servers[i].locations[j].method << "\n";
            std::cout << "      Root: " << servers[i].locations[j].root << "\n";
        }
    }

    return 0;
}
