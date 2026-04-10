#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include "ServerConfig.hpp"
#include "Parser.hpp"
#include <iostream>

class Server {
    private:
        std::vector<ServerConfig> configs;
    public:
        Server();
        ~Server();
        int init(const std::string& configFile);
};

#endif 