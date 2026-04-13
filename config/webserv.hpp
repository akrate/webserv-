#ifndef WEBSERV_HPP
#define WEBSERV_HPP



#include <string>
#include <iostream>
#include <vector>
#include "LocationConfig.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

struct LocationConfig 
{
    std::string              path;
    std::string              root;
    std::vector<std::string> index;
    std::vector<std::string> allowed_methods;
    bool                     autoindex;
    int                      redirect_code;
    std::string              redirect_url;
    std::vector<std::string> cgi_extensions;
    std::string              cgi_path;
    std::string              upload_store;
    bool                     allow_upload;
    size_t                   client_max_body_size;
	
};
struct ServerConfig
{
	std::string server_name;
	int port;
	std::string root;
	std::vector<std::string> index;
	std::map<int, std::string> error_pages;
	size_t client_max_body_size;
    std::vector<LocationConfig> locations;
};

namespace Utils
{
	std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str);

}
#endif