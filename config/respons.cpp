#include "response.hpp"

Response::Response() :
    status_code(200),
    status_message("OK")
{
    headers["Server"] = "WebServ/1.0";
    headers["Connection"] = "close";
}
std::string Response::toString() const
{
    std::stringstream ss;
    std::string spliter = "\r\n";
    ss << "HTTP/1.1 " << status_code << " " << status_message << spliter;

    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
    {
        ss << it->first << ": " << it->second << spliter;
    }
    ss << spliter;
    ss << body;
    return ss.str();
}

void Response::setStatusCode(int code)
{
    status_code = code;
    status_message = getMessageBycode(code);
}

void Response::addHeader(const std::string& key, const std::string& value) 
{
    std::string lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
    
    headers[lowerKey] = value;
}
void Response::setBody(const std::string& body_content)
{
    body = body_content;
    std::stringstream ss;
    ss << body.size();
    addHeader("content-length",ss.str());
}