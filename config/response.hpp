#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Server.hpp"

class Response
{
    private:
        int status_code;
        std::string status_message; 
        std::string body;
        std::map<std::string, std::string> headers;
        std::string getMessageBycode(int code) const;
    
    public:
        Response();
        std::string toString() const;
        void setStatusCode(int code);
        void setBody(const std::string& body_content);
        void addHeader(const std::string& key, const std::string& value);
        std::string getMediaType(const std::string& extencion);
};
Response build_response(const HttpRequest& req,
                           const ServerConfig& config,
                           const LocationConfig& location);
Response build_page_error(const int code);
#endif