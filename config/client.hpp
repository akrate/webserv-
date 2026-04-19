#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "webserv.hpp"

class Client
{
public:
    Client();
    std::string raw_request;
    HttpRequest request;
    bool request_complete;
    bool headers_parsed;
    void parse_request();
    void parse_request_line(const std::string& line);
};

#endif