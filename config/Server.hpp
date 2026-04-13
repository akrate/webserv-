#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.hpp"
#include "Parser.hpp"

class Server {
    private:
        std::vector<ServerConfig> configs;
    public:
        Server();
        ~Server();
        int init(const std::string& configFile);
};

#endif 