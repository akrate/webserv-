#include "Parser.hpp"
#include "ServerConfig.hpp"
#include "Server.hpp"
#include <iostream>
#include <fstream>


int main(int ac, char **av){

    if (ac != 2){
        std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
        return 1;
    }
    Server server;
    if(server.init(av[1]) != 0){
        std::cerr << "Failed to initialize server with config file: " << av[1] << std::endl;
        return 1;
    }
}