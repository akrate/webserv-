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
        std::string getMessageBycode(int code) const
        {
            switch (code)
            {
                case 200:
                    return "OK";
                case 201:
                    return "Created";
                case 400:
                    return "Bad Request";
                case 403:
                    return "Forbidden";
                case 404:
                    return "Not Found";
                case 405:
                    return "Method Not Allowed";
                case 500:
                    return "Internal Server Error";
                case 505:
                    return "HTTP Version Not Supported";
                default:
                    return "Internal Server Error";
            }
        }
    
    public:
        std::string toString() const;
        void setStatusCode(int code);
        void setBody(const std::string& body_content);
        void addHeader(const std::string& key, const std::string& value);
};
#endif