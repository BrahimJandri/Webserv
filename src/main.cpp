#include "./Parser/ConfigParser.hpp"
#include "./Utils/Logger.hpp"
#include "Utils/AnsiColor.hpp"
#include "Server/Server.hpp"

void printBanner() {
    Utils::log(" █     █░▓█████  ▄▄▄▄     ██████ ▓█████  ██▀███   ██▒   █▓", AnsiColor::GREEN);
    Utils::log("▓█░ █ ░█░▓█   ▀ ▓█████▄ ▒██    ▒ ▓█   ▀ ▓██ ▒ ██▒▓██░   █▒", AnsiColor::GREEN);
    Utils::log("▒█░ █ ░█ ▒███   ▒██▒ ▄██░ ▓██▄   ▒███   ▓██ ░▄█ ▒ ▓██  █▒░", AnsiColor::GREEN);
    Utils::log("░█░ █ ░█ ▒▓█  ▄ ▒██░█▀    ▒   ██▒▒▓█  ▄ ▒██▀▀█▄    ▒██ █░░", AnsiColor::GREEN);
    Utils::log("░░██▒██▓ ░▒████▒░▓█  ▀█▓▒██████▒▒░▒████▒░██▓ ▒██▒   ▒▀█░  ", AnsiColor::GREEN);
    Utils::log("░ ▓░▒ ▒  ░░ ▒░ ░░▒▓███▀▒▒ ▒▓▒ ▒ ░░░ ▒░ ░░ ▒▓ ░▒▓░   ░ ▐░  ", AnsiColor::GREEN);
    Utils::log("  ▒ ░ ░   ░ ░  ░▒░▒   ░ ░ ░▒  ░ ░ ░ ░  ░  ░▒ ░ ▒░   ░ ░░  ", AnsiColor::GREEN);
    Utils::log("  ░   ░     ░    ░    ░ ░  ░  ░     ░     ░░   ░      ░░  ", AnsiColor::GREEN);
    Utils::log("    ░       ░  ░ ░            ░     ░  ░   ░           ░  ", AnsiColor::GREEN);
    Utils::log("                      ░                               ░   ", AnsiColor::GREEN);
}

int start_server(const std::string &config_path)
{
    printBanner();
    Utils::log("Starting Webserv with configuration: " + config_path, AnsiColor::GREEN);
    ConfigParser parser;
    try
    {
        parser.parseFile(config_path);
    }
    catch (const std::runtime_error &e)
    {
        Utils::log("Error parsing configuration: " + std::string(e.what()), AnsiColor::RED);
        return 1;
    }
    size_t serverCount = parser.getServerCount();
    Utils::log("Found " + Utils::intToString(serverCount) + " server configurations", AnsiColor::CYAN);
    parser.printConfig();
    Server server;
    server.setupServers(parser); // 👈 create sockets + register to epoll
    server.handleConnections();  // 👈 single loop to handle all
    server.Cleanup();            // 👈 cleanup all clients and sockets
    std::cout << AnsiColor::BOLD_RED << "Webserv stopped" << AnsiColor::RESET << std::endl;
    return 0;
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