#include "Server.hpp"

Server::Server() {
}

Server::~Server() {
}

int Server::init(const std::string& configFile) {
    configs = Parser::parse(configFile);
    if(configs.empty()) {
        std::cerr << "Error: No valid server configurations found in " << configFile << std::endl;
        return -1;
    }
    return 0;
}