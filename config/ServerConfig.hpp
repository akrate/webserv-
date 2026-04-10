#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>

class ServerConfig {
    public:
        std::string host;
        int port;
        std::vector<std::string> server_names;
        std::vector<std::string> allowed_methods;
        std::vector<std::string> error_pages;
        std::vector<std::string> locations;
};


#endif