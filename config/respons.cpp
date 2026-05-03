#include "response.hpp" 

bool isDirectory(const std::string &path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == 0)
        return S_ISDIR(s.st_mode);
    return false;
}

Response::Response() :
    status_code(200),
    status_message("OK")
{
    headers["server"] = "WebServ/1.0";
    headers["connection"] = "close";
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
    if (extension == "html" || extension == "htm")
        return "text/html";
    if (extension == "css")
        return "text/css";
    if (extension == "js")
        return "application/javascript";
    if (extension == "png")
        return "image/png";
    if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    if (extension == "gif")
        return "image/gif";
    if (extension == "txt")
        return "text/plain";
    if (extension == "pdf")
        return "application/pdf";
    if (extension == "ico")
        return "image/x-icon";
    
    return "application/octet-stream";
}

Response build_response(const HttpRequest& req,
                        const ServerConfig& config,
                        const LocationConfig& location)
{
    Response res;
    (void)config;
    std::string path;
    if (req.path == "/")
        path = location.root + "/index.html";
    else
        path = location.root + req.path;
    std::cout << "===>" << path << std::endl;
    if (isDirectory(path) == true)
    {
        res.setStatusCode(404);
        std::ifstream file("./www/html/errors/404.html");
        std::string body((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
        res.addHeader("content-type", res.getMediaType(getExtension(path)));
        res.setBody(body);
        return res;
    }
    // =======================
    // 1. GET METHOD
    // =======================
    if (req.method == "GET")
    {
        if (!location.isMethodAllowed(req.method))
        {
            res.setStatusCode(405);
            res.setBody("Method Not Allowed");
            return res;
        }
        std::ifstream file(path.c_str());
        if (!file.is_open())
        {
            res.setStatusCode(404);
            std::ifstream file("./www/html/errors/404.html");
            std::string body((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
            res.addHeader("content-type", res.getMediaType(getExtension(path)));
            res.setBody(body);
            return res;
        }

        std::string body((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

        res.setStatusCode(200);
        res.addHeader("content-type", res.getMediaType(getExtension(path)));
        res.setBody(body);
        return res;
    }

    // =======================
    // 2. POST METHOD
    // =======================
    else if (req.method == "POST")
    {
        // simulate saving body to file
        if (!location.isMethodAllowed(req.method))
        {
            res.setStatusCode(405);
            res.setBody("Method Not Allowed");
            return res;
        }
        std::ofstream file(path.c_str(), std::ios::app);
        if (!file.is_open())
        {
            res.setStatusCode(500);
            res.setBody("Internal Server Error");
            return res;
        }

        file << req.body; // body from request

        res.setStatusCode(201);
        res.setBody("Created");
        return res;
    }

    // =======================
    // 3. DELETE METHOD
    // =======================
    else if (req.method == "DELETE")
    {
        if (!location.isMethodAllowed(req.method))
        {
            res.setStatusCode(405);
            res.setBody("Method Not Allowed");
            return res;
        }
        if (std::remove(path.c_str()) != 0)
        {
            res.setStatusCode(404);
            res.setBody("File Not Found");
            return res;
        }

        res.setStatusCode(200);
        res.setBody("Deleted");
        return res;
    }

    // =======================
    // 4. METHOD NOT ALLOWED
    // =======================
    res.setStatusCode(405);
    res.setBody("Method Not Allowed");
    return res;
}


Response build_page_error(const int code)
{
    Response res;
    if (code == 400)
    {
        std::ifstream file("../page_error/404.html");
        std::string body((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
        res.addHeader("content-type", res.getMediaType("html"));
        res.setBody(body);
    }
    if (code == 405)
    {
        std::ifstream file("../page_error/404.html");
        std::string body((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
        res.addHeader("content-type", res.getMediaType("html"));
        res.setBody(body);
    }
    if (code == 505)
    {
        std::ifstream file("../page_error/504.html");
        std::string body((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
        res.addHeader("content-type", res.getMediaType("html"));
        res.setBody(body);
    }
    res.setStatusCode(code);
    res.addHeader("Connection", "close");
    return res;
}