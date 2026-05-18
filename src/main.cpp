#include "../include/Server.hpp"
#include <iostream>
#include <string>

bool hasConfExtension(const std::string& file)
{
    if (file.length() < 5)
        return false;
    return file.substr(file.length() - 5) == ".conf";
}

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: ./webserv <config_file>" << std::endl;
            return 1;
        }
        if (!hasConfExtension(argv[1]))
        {
            std::cerr << "Error: config file must end with .conf" << std::endl;
            return 1;
        }

        Server server;
        if (server.init(argv[1]) < 0) {
            std::cerr << "Failed to initialize server" << std::endl;
            return 1;
        }

        std::cout << "WebServ started successfully" << std::endl;
        server.run();
    }
    catch (ParserException& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}