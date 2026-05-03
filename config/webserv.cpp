#include "webserv.hpp"
LocationConfig::LocationConfig()
    : autoindex(false),
    redirect_code(0),
    allow_upload(false),
    client_max_body_size(0)
{}


ServerConfig::ServerConfig()
    : port(0),
    host("0.0.0.0"),
    client_max_body_size(1024 * 1024)
{}

