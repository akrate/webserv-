#include "../include/response.hpp"

bool isDirectory(const std::string &path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == 0)
        return S_ISDIR(s.st_mode);
    return false;
}

std::string getExtension(const std::string& path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "";
    return path.substr(dot + 1);
}

Response::Response() :
    status_code(200),
    status_message("OK")
{
    headers["server"]     = "WebServ/1.0";
    headers["connection"] = "close";
}

std::string Response::getMessageBycode(int code) const
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 505: return "HTTP Version Not Supported";
        default:  return "Internal Server Error";
    }
}

std::string Response::toString() const
{
    std::stringstream ss;
    ss << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
        ss << it->first << ": " << it->second << "\r\n";
    ss << "\r\n";
    ss << body;
    return ss.str();
}

void Response::setStatusCode(int code)
{
    status_code    = code;
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
    addHeader("content-length", ss.str());
}

std::string Response::getMediaType(const std::string& extension)
{
    if (extension == "html" || extension == "htm") return "text/html";
    if (extension == "css")                        return "text/css";
    if (extension == "js")                         return "application/javascript";
    if (extension == "png")                        return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif")                        return "image/gif";
    if (extension == "txt")                        return "text/plain";
    if (extension == "pdf")                        return "application/pdf";
    if (extension == "ico")                        return "image/x-icon";
    return "application/octet-stream";
}

Response build_page_error(int code)
{
    Response res;

    std::string path = "./www/html/errors/";
    std::stringstream ss;
    ss << code;
    path += ss.str() + ".html";

    std::ifstream file(path.c_str());
    std::string body;
    if (file.is_open())
        body.assign(std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>());
    else
    {
        body = "<html><body><h1>" + ss.str() + " " +
               res.getMessageBycode(code) + "</h1></body></html>";
    }

    res.setStatusCode(code);
    res.addHeader("content-type", res.getMediaType("html"));
    res.addHeader("connection", "close");
    res.setBody(body);
    return res;
}

Response build_response(const HttpRequest& req,
                        const ServerConfig& config,
                        const LocationConfig& location)
{
    Response res;
    std::string path;

    if (!location.redirect_url.empty())
    {
        int code = location.redirect_code;
        if (code < 300 || code >= 400) code = 301;
        res.setStatusCode(code);
        res.addHeader("location", location.redirect_url);
        std::stringstream ss;
        ss << code;
        res.setBody("<html><body><h1>" + ss.str() + " Redirect</h1>"
                    "<p>The document has moved <a href=\"" +
                    location.redirect_url + "\">here</a>.</p></body></html>");
        res.addHeader("content-type", "text/html");
        return res;
    }

    if (!location.isMethodAllowed(req.method))
    {
        res = build_page_error(405);
        std::string allow;
        for (size_t i = 0; i < location.allowed_methods.size(); i++)
        {
            allow += location.allowed_methods[i];
            if (i < location.allowed_methods.size() - 1)
                allow += ", ";
        }
        res.addHeader("allow", allow);
        return res;
    }

    if (req.path == "/")
    {
        std::string root = location.root;
        if (!root.empty() && root[root.size() - 1] != '/')
            root += "/";

        bool found = false;
        for (size_t i = 0; i < location.index.size(); i++)
        {
            std::string candidate = root + location.index[i];
            if (access(candidate.c_str(), R_OK) == 0)
            {
                path  = candidate;
                found = true;
                break;
            }
        }
        if (!found)
        {
            for (size_t i = 0; i < config.index.size(); i++)
            {
                std::string candidate = root + config.index[i];
                if (access(candidate.c_str(), R_OK) == 0)
                {
                    path  = candidate;
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            if (location.autoindex)
                return generate_autoindex(location.root);
            return build_page_error(404);
        }
    }
    else
        path = location.root + req.path;

    if (isDirectory(path))
    {
        for (size_t i = 0; i < location.index.size(); i++)
        {
            std::string candidate = path + "/" + location.index[i];
            std::ifstream test(candidate.c_str());
            if (test.is_open())
            {
                path = candidate;
                break;
            }
        }
        if (isDirectory(path))
        {
            if (location.autoindex)
                return generate_autoindex(path);
            return build_page_error(403);
        }
    }

    if (req.method == "GET")
    {
        std::ifstream file(path.c_str());
        if (!file.is_open())
            return build_page_error(404);

        std::string body((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        res.setStatusCode(200);
        res.addHeader("content-type", res.getMediaType(getExtension(path)));
        res.setBody(body);
        return res;
    }
    else if (req.method == "POST")
    {
        std::string filename;
        std::string key = "filename=";
        size_t pos = req.query.find(key);
        if (pos != std::string::npos)
            filename = req.query.substr(pos + key.size());
        if (filename.empty())
            filename = "upload.txt";

        std::string upload_path = location.upload_store + "/" + filename;
        std::ofstream file(upload_path.c_str(), std::ios::trunc);
        if (!file.is_open())
            return build_page_error(500);

        file << req.body;
        res.setStatusCode(201);
        res.setBody("Created");
        return res;
    }
    else if (req.method == "DELETE")
    {
        if (std::remove(path.c_str()) != 0)
            return build_page_error(404);
        res.setStatusCode(200);
        res.setBody("Deleted");
        return res;
    }

    return build_page_error(404);
}

std::vector<std::string> list_files(const std::string& path)
{
    std::vector<std::string> files;
    DIR *dir = opendir(path.c_str());
    if (!dir) return files;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name(entry->d_name);
        if (name == "." || name == "..") continue;
        files.push_back(name);
    }
    closedir(dir);
    return files;
}

Response generate_autoindex(const std::string& dir)
{
    Response res;
    std::vector<std::string> files = list_files(dir);

    std::string body;
    body += "<html><head><title>Index</title></head><body>";
    body += "<h1>Index of " + dir + "</h1>";
    body += "<a href=\"../\">../</a><br>";
    for (size_t i = 0; i < files.size(); i++)
        body += "<a href=\"" + files[i] + "\">" + files[i] + "</a><br>";
    body += "</body></html>";

    res.setStatusCode(200);
    res.addHeader("content-type", "text/html");
    res.setBody(body);
    return res;
}