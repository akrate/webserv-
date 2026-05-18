#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "webserv.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
class Response;
class Client
{
public:
    Client();
    std::string raw_request;
    HttpRequest request;
    void append_data(const std::string& data, const ServerConfig& conf);
    bool is_complete() const;
    void parse_request(const ServerConfig& conf);
    const HttpRequest& getRequest() const;
    int getErrorCode() const;
    void reset();                // ← add this
private:
    bool   request_complete;
    bool   headers_parsed;
    bool   is_chunked;
    size_t content_length;
    int    error_code;
    void parse_request_line(const std::string& line);
    void parse_body();
    void parse_chunked_body();
    void parse_headers(const std::string& headers_part);
    std::string send_buffer;
    size_t bytes_sent;
};
std::string getExtensionclient(const std::string& path);

#endif