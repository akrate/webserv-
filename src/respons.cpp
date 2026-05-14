#include "../include/response.hpp"

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
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content"; // مهمة فـ DELETE إلا نجح وما كاين body
        case 301: return "Moved Permanently"; // إلا درتي Redirection
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Content Too Large"; // مهمة للـ client_max_body_size
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 504: return "Gateway Timeout"; // مهمة للـ CGI Timeout اللي قاديتي
        case 505: return "HTTP Version Not Supported";
        default:  return "Internal Server Error"; // اختيار آمن
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
   if (!location.redirect_url.empty())
    {
        int code = location.redirect_code;

        if (code < 300 || code >= 400)
            code = 301; 
        res.setStatusCode(code);
        res.addHeader("Location", location.redirect_url);
        std::stringstream ss;
        ss << code;
        std::string codeStr = ss.str();
        res.setBody("<html><body><h1>" + codeStr + " Redirect</h1>" +
                    "<p>The document has moved <a href=\"" + location.redirect_url + "\">here</a>.</p>" +
                    "</body></html>");
        res.addHeader("Content-Type", "text/html");
        return res;
    }
    if (!location.isMethodAllowed(req.method))
    {
        res = build_page_error(405,config,location);
        std::string allow;
        for (size_t i = 0; i < location.allowed_methods.size();i++)
        {
            allow += location.allowed_methods[i];
            if (i < location.allowed_methods.size() - 1)
                allow += ", ";
        }
        res.addHeader("allow",allow);
        return  res;
    }
    if (req.path == "/")
    {
        std::string root = location.root;
        bool found = false;
        if (!root.empty() && root[root.size() - 1] != '/')
            root += "/";
        for (size_t i = 0; i < location.index.size(); i++) 
        {
            std::string candidate = root + location.index[i];
            if (access(candidate.c_str(), R_OK) == 0) 
            {
                path = candidate;
                found = true;
                break;
            }
        }
        if (found == false)
        {
            for (size_t i = 0; i < config.index.size(); i++)
            {
                std::string candidate = root + config.index[i];
                if (access(candidate.c_str(), R_OK) == 0) 
                {
                    path = candidate;
                    found = true;
                    break;
                }
            }
        }
        if (found == false)
        {
            if (location.autoindex)
            {
                return generate_autoindex(location.root);
            }
             return build_page_error(404,config,location);
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

            
            return build_page_error(403,config,location);
        }
    }
    // if (isCgi(path))
    // {
    //     std::cout << "\033[31mcgi==>""\033[0m"<< path << std::endl;
    //     return res.execute_cgi(req, path, location);
    // }
    if (req.method == "GET")
    {
        std::ifstream file(path.c_str());
        if (!file.is_open())
        {
            return build_page_error(404,config,location);
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
            return build_page_error(500,config,location); 
        }
        file << req.body;
        res.setStatusCode(201);
        res.setBody("Created");
        return res;
    }
    else if (req.method == "DELETE")
    {
        if (std::remove(path.c_str()) != 0)
        {
            return build_page_error(404,config,location);
        }
        res.setStatusCode(200);
        res.setBody("Deleted");
        return res;
    }
    return  build_page_error(404,config,location);
}

// Response build_page_error(int code)
// {
//     Response res;
//     res.setStatusCode(code);
//     res.addHeader("Content-Type", "text/html");

//     std::string message;
//     if (code == 400) message = "Bad Request";
//     else if (code == 403) message = "Forbidden";
//     else if (code == 404) message = "Not Found";
//     else if (code == 405) message = "Method Not Allowed";
//     else if (code == 500) message = "Internal Server Error";
//     else message = "Error";

//     std::string error_page_path = "./www/html/errors/" + to_string(code) + ".html";
//     std::ifstream file(error_page_path.c_str());
    
//     if (file.is_open())
//     {
//         std::string body((std::istreambuf_iterator<char>(file)),
//                           std::istreambuf_iterator<char>());
//         res.setBody(body);
//     }
//     else
//     {
//         res.setBody("<html><body><h1>" + to_string(code) + " " + message + "</h1>" +
//                     "<p>We're sorry, but an error occurred.</p></body></html>");
//     }
    
//     return res;
// }
// Response build_page_error(const int code, const ServerConfig& config, const LocationConfig& location)

// Response build_page_error(const int code)
// {
//     Response res;
//     if (code == 400)
//     {
//         std::ifstream file("./www/html/errors/400.html");
//         std::string body((std::istreambuf_iterator<char>(file)),
//         std::istreambuf_iterator<char>());
//         res.addHeader("content-type", res.getMediaType("html"));
//         res.setBody(body);
//     }
//     if (code == 500)
//     {
//         std::ifstream file("./www/html/errors/500.html");
//         std::string body((std::istreambuf_iterator<char>(file)),
//         std::istreambuf_iterator<char>());
//         res.addHeader("content-type", res.getMediaType("html"));
//         res.setBody(body);
//     }
//     if (code == 405)
//     {
//         std::ifstream file("./www/html/errors/405.html");
//         std::string body((std::istreambuf_iterator<char>(file)),
//         std::istreambuf_iterator<char>());
//         res.addHeader("content-type", res.getMediaType("html"));
//         res.setBody(body);
//     }
//     if (code == 505)
//     {
//         std::ifstream file("./www/html/errors/505.html");
//         std::string body((std::istreambuf_iterator<char>(file)),
//         std::istreambuf_iterator<char>());
//         res.addHeader("content-type", res.getMediaType("html"));
//         res.setBody(body);
//     }
//     if (code == 413)
//     {
//         std::ifstream file("./www/html/errors/413.html");
//         std::string body((std::istreambuf_iterator<char>(file)),
//         std::istreambuf_iterator<char>());
//         res.addHeader("content-type", res.getMediaType("html"));
//         res.setBody(body);
//     }
//      if (code == 502)
//     {
//         std::ifstream file("./www/html/errors/502.html");
//         std::string body((std::istreambuf_iterator<char>(file)),
//         std::istreambuf_iterator<char>());
//         res.addHeader("content-type", res.getMediaType("html"));
//         res.setBody(body);
//     }
//     res.setStatusCode(code);
//     res.addHeader("Connection", "close");
//     return res;
// }
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
std::string to_str(int n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}
Response build_page_error(int code, const ServerConfig& config, const LocationConfig& location)
{
    Response res;
    std::string body;
    std::string path = "";
    std::string uri = "";

    std::map<int, std::string>::const_iterator it_loc = location.error_pages.find(code);
    if (it_loc != location.error_pages.end()) {
        uri = it_loc->second;
    }
    else {
        std::map<int, std::string>::const_iterator it_serv = config.error_pages.find(code);
        if (it_serv != config.error_pages.end()) {
            uri = it_serv->second;
        }
    }
    if (!uri.empty()) {
        std::string current_root = (!location.root.empty()) ? location.root : config.root;
        path = current_root + uri;
        std::cout << "\033[32m" << path << "\033[0m" << std::endl;
        std::ifstream file(path.c_str());
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            body = buffer.str();
            file.close();
        }
    }
    if (body.empty()) {
        std::stringstream ss;
        ss << "<html><head><title>" << code << " " << res.getMessageBycode(code) << "</title></head>"
           << "<body style='font-family: Arial, sans-serif; text-align: center; margin-top: 50px;'>"
           << "<h1>" << code << " " << res.getMessageBycode(code) << "</h1>"
           << "<hr style='width: 50%;'>"
           << "<p><i>webserv/1.0</i></p>"
           << "</body></html>";
        body = ss.str();
    }
    res.setStatusCode(code);
    res.setBody(body);
    res.addHeader("Content-Type", "text/html");
    res.addHeader("Connection", "close");

    return res;
}