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
    std::string path;
    if (req.path == "/")
    {
        for (size_t i = 0; i < config.index.size();i++)
        {
            std::string candidate = location.root + config.index[i];
            std::ifstream test(candidate.c_str());
            if (test.is_open())
            {
                path = candidate;
                break;
            }
        }
        if (path.empty())
        {
            if (location.autoindex)
            {
                return generate_autoindex(location.root);
            }
            res.setStatusCode(404);
            std::ifstream file("./www/html/errors/404.html");
            std::string body((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
            res.addHeader("content-type", res.getMediaType("html"));
            res.setBody(body);
            return res;
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

            res.setStatusCode(403);
            res.setBody("Forbidden");
            return res;
        }
    }
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
    else if (req.method == "POST")
    {
        if (!location.isMethodAllowed(req.method))
        {
            return build_page_error(405);   
        }

        std::string filename;
        std::string query = req.query; // or however you access it
        std::string key = "filename=";
        size_t pos = query.find(key);
        if (pos != std::string::npos)
            filename = query.substr(pos + key.size());

        if (filename.empty())
            filename = "upload.txt";

        std::string upload_path = location.upload_store + "/" + filename;
        std::cout << "==> Writing to: " << upload_path << std::endl;

        std::ofstream file(upload_path.c_str(), std::ios::trunc);
        if (!file.is_open())
        {
            return build_page_error(500); 
        }

        file << req.body;

        res.setStatusCode(201);
        res.setBody("Created");
        return res;
    }
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
            return build_page_error(404);
        }

        res.setStatusCode(200);
        res.setBody("Deleted");
        return res;
    }
    return build_page_error(405);
}

Response build_page_error(const int code)
{
    Response res;
    if (code == 400)
    {
        std::ifstream file("./www/html/errors/400.html");
        std::string body((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
        res.addHeader("content-type", res.getMediaType("html"));
        res.setBody(body);
    }
    if (code == 405)
    {
        std::ifstream file("./www/html/errors/405.html");
        std::string body((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
        res.addHeader("content-type", res.getMediaType("html"));
        res.setBody(body);
    }
    if (code == 505)
    {
        std::ifstream file("./www/html/errors/505.html");
        std::string body((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
        res.addHeader("content-type", res.getMediaType("html"));
        res.setBody(body);
    }
    if (code == 413)
    {
        std::ifstream file("./www/html/errors/413.html");
        std::string body((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
        res.addHeader("content-type", res.getMediaType("html"));
        res.setBody(body);
    }
    res.setStatusCode(code);
    res.addHeader("Connection", "close");
    return res;
}
std::vector<std::string> list_files(const std::string& path)
{
    std::vector<std::string> files;

    DIR *dir = opendir(path.c_str());
    if (!dir)
        return files;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name(entry->d_name);

        if (name == "." || name == "..")
            continue;

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
    {
        body += "<a href=\"" + files[i] + "\">" + files[i] + "</a><br>";
    }

    body += "</body></html>";

    res.setStatusCode(200);
    res.addHeader("Content-Type", "text/html");
    res.setBody(body);

    return res;
}