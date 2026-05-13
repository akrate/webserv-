#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include "LocationConfig.hpp"

class ServerConfig
{
    public:
        ServerConfig();                     
        std::vector<std::string>    server_names;
        int                         port;
        std::string                 root;
        std::string                 host;
        std::vector<std::string>    index;
        std::map<int, std::string>  error_pages;
        size_t                      client_max_body_size;
        std::vector<LocationConfig> locations;

        
};

#endif