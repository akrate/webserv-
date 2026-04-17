#ifndef PARSER_HPP
#define PARSER_HPP  

#include "webserv.hpp"
#include <vector>
#include <algorithm>

class Parser {
    public:
        Parser();
        ~Parser();
        static std::vector<ServerConfig> parse(const std::string& configFile);
    private:
        static std::string read_File(const std::string& path);
        static void remouve_comments(std::string& content);
        static ServerConfig parse_server_block(const std::string& content, size_t& pos);
        static std::string extarct_block(const std::string& content, size_t& pos);
        static LocationConfig parse_location_block(const std::string& content, size_t& pos, std::string& path);
        static void validate_brackets(const std::string& content);
        static void validate_semicolons(const std::string& content);
        static void validate_structure(const std::string& content);

};

#endif