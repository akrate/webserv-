#ifndef WEBSERV_HPP
#define WEBSERV_HPP



#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
// #include "response.hpp"


class LocationConfig 
{
    public:                          
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
        bool isMethodAllowed(const std::string& method) const
        {
            for (size_t i = 0; i < allowed_methods.size(); i++)
            {
                if (allowed_methods[i] == method)
                    return true;
            }
            return false;
        }
        void print() const
    {
        std::cout << "===== LocationConfig =====" << std::endl;

        std::cout << "path: " << path << std::endl;
        std::cout << "root: " << root << std::endl;

        std::cout << "index: ";
        for (size_t i = 0; i < index.size(); i++)
            std::cout << index[i] << " ";
        std::cout << std::endl;

        std::cout << "allowed_methods: ";
        for (size_t i = 0; i < allowed_methods.size(); i++)
            std::cout << allowed_methods[i] << " ";
        std::cout << std::endl;

        std::cout << "autoindex: " << (autoindex ? "on" : "off") << std::endl;

        std::cout << "redirect_code: " << redirect_code << std::endl;
        std::cout << "redirect_url: " << redirect_url << std::endl;

        std::cout << "cgi_extensions: ";
        for (size_t i = 0; i < cgi_extensions.size(); i++)
            std::cout << cgi_extensions[i] << " ";
        std::cout << std::endl;

        std::cout << "cgi_path: " << cgi_path << std::endl;
        std::cout << "upload_store: " << upload_store << std::endl;

        std::cout << "allow_upload: " << (allow_upload ? "true" : "false") << std::endl;

        std::cout << "client_max_body_size: " << client_max_body_size << std::endl;

        std::cout << "==========================" << std::endl;
    }
      
};

class ServerConfig
{
    public:                          
        std::vector<std::string>    server_names;
        int                         port;
        std::string                 root;
        std::string                 host;
        std::vector<std::string>    index;
        std::map<int, std::string>  error_pages;
        size_t                      client_max_body_size;
        std::vector<LocationConfig> locations;

        
};

class HttpRequest
{
    public:
        std::string method;
        std::string path;
        std::string version;
        std::string body;
        std::string query;
	    std::map<std::string, std::string> headers;

};

namespace Utils
{
	std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str);
    std::string to_upper(const std::string& str);
    void trim2(std::string& str);
}
#endif