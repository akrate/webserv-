#include "response.hpp" 

Response::Response() :
    status_code(200),
    status_message("OK")
{
    headers["Server"] = "WebServ/1.0";
    headers["Connection"] = "close";
}

std::string Response::getMessageBycode(int code) const
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

std::string Response::getMediaType(const std::string& extension)
{
    if (extension == "html" || extension == "htm") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js")  return "application/javascript";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    if (extension == "txt") return "text/plain";
    if (extension == "pdf") return "application/pdf";
    if (extension == "ico") return "image/x-icon";
    
    return "application/octet-stream";
}
