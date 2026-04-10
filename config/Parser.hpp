#ifndef PARSER_HPP
#define PARSER_HPP  

#include <vector>
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

class Parser {
    public:
        Parser();
        ~Parser();
        static std::vector<ServerConfig> parse(const std::string& configFile);
    private:
        static std::string read_File(const std::string& path);
        static void remouve_comments(std::string& content);
        static ServerConfig parse_server_block(const std::string& content, size_t& pos);
};

#endif