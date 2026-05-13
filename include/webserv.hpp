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
}
#endif