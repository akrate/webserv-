#include "client.hpp"

Client::Client()
    : request_complete(false),
      headers_parsed(false),
      is_chunked(false),
      content_length(0),
      error_code(0)
{}
const HttpRequest& Client::getRequest() const
{
    return request;
}

int Client::getErrorCode() const
{
    return error_code;
}
bool Client::is_complete() const 
{ 
    return request_complete; 
}
void Client::append_data(const std::string& data)
{
    raw_request += data;
    parse_request();
}
void Client::parse_request()
{
    if(request_complete || error_code != 0)
        return;
    size_t pos = raw_request.find("\r\n\r\n");
    if(pos == std::string::npos)
        return;
    if(!headers_parsed)
    {
        std::string headers_part = raw_request.substr(0, pos);
        std::istringstream ss(headers_part);
        std::string line;

        std::getline(ss, line);
        parse_request_line(line);
        if(error_code != 0)
            return;
        size_t first_line_end = headers_part.find("\r\n");
        if (first_line_end == std::string::npos)
        {
            error_code = 400;
            return;
        }
        parse_headers(headers_part.substr(first_line_end + 2));
        headers_parsed = true;

        if(request.headers.count("content-length"))
        {
            char *end;
            long val = strtol(request.headers["content-length"].c_str(), &end, 10);
            if(*end != '\0' || val < 0)
            {
                error_code = 400;
                return;
            }
            content_length = (size_t)val;
        }
        if(request.headers.count("transfer-encoding") && request.headers["transfer-encoding"] == "chunked")
            is_chunked = true;
    }
    // if(is_chunked)
    //     parse_chunked_body();
    
    parse_body();
    

}

void Client::parse_request_line(const std::string &line)
{
    std::istringstream ss(line);
    std::string method, path_query, version;

    ss >> method >> path_query >> version;
    if(method.empty() || path_query.empty() || version.empty())
    {
        error_code = 400; // EMPTY ELEMENT 
        return;
    }

    if(version != "HTTP/1.0" && version != "HTTP/1.1")
    {
        error_code = 505 ;// VERSION NOT SUPPORTED
        return;
    }
    if(method != "POST" && method != "GET" && method != "DELETE")
    {
        error_code = 405;// METHOD NOT ALLOWED
        return;
    }
    request.method = method;
    request.version = version;

    size_t q = path_query.find('?');
    if(q != std::string::npos)
    {
        request.path = path_query.substr(0, q);
        request.query = path_query.substr(q + 1);
    }
    else
    {
        request.path = path_query;
        request.query = "";
    }
}

void Client::parse_headers(const std::string& headers_part)
{
    std::istringstream ss(headers_part);
    std::string line;

    while(std::getline(ss, line))
    {
        if(!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if(line.empty())
            break;
        size_t colon = line.find(':');
        if(colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        Utils::trim2(key);
        Utils::trim2(value);

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        request.headers[key] = value;
    }
}

void Client::parse_body()
{
    size_t pos = raw_request.find("\r\n\r\n");
    if (pos == std::string::npos)
        return;
    std::string body = raw_request.substr(pos + 4);

    if(request.method == "GET" || request.method == "DELETE")
    {
        request_complete = true;
        return;
    }
    if(content_length == 0)
    {
        request_complete = true;
        return;
    }
    if(body.size() < content_length)
        return;
    request.body = body.substr(0, content_length);
    request_complete = true;
}

