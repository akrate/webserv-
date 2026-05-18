#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Server.hpp"
#include "client.hpp"
#include "cgi_process.hpp"         // ← add this
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sstream>
class Response
{
    private:
        int                                 status_code;
        std::string                         status_message;
        std::string                         body;
        std::map<std::string, std::string>  headers;

    public:
        Response();
        std::string toString() const;
        void        setStatusCode(int code);
        void        setBody(const std::string& body_content);
        void        addHeader(const std::string& key, const std::string& value);
        std::string getMediaType(const std::string& extension);
        std::string                         getMessageBycode(int code) const;
        static void parseCgiOutput(const std::string& cgi_output, Response& res);
};

Response                 build_response(const HttpRequest& req,
                                        const ServerConfig& config,
                                        const LocationConfig& location);
Response build_page_error(const int code, const ServerConfig& config, const LocationConfig& location);


Response                 generate_autoindex(const std::string& dir);
std::vector<std::string> list_files(const std::string& path);
bool                     isCgi(const std::string& path);
char**                   prepareEnv(const HttpRequest& req, const std::string& scriptPath);
void                     freeEnv(char** env);

#endif