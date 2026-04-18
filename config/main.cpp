#include "Server.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: ./webserv <config_file>" << std::endl;
        return 1;
    }
    Server server;
    if (server.init(argv[1]) < 0) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }
    std::cout << "WebServ started successfully" << std::endl;
    server.run();
    return 0;
}