#ifndef CLIENT_HPP
#define CLIENT_HPP

// #include "webserv.hpp"
#include "response.hpp"

class Client
{
public:
    Client();
    std::string raw_request;
    HttpRequest request;
    void append_data(const std::string& data);
    bool is_complete() const;
    void parse_request();
    const HttpRequest& getRequest() const;
    int getErrorCode() const;
    //////////////////////////////////////////oussama
    void prepareResponse(const Response& res);
    // bool handleSend(int socket_fd);
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
    //////////////////////////////////////////// oussama
    std::string send_buffer;
    size_t bytes_sent;
};
std::string getExtension(const std::string& path);


#endif