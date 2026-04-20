#include "client.hpp"

Client::Client(): request_complete(false), headers_parsed(false)
{

}

void Client::parse_request()
{
    if(request_complete)
        return;
    size_t pos = raw_request.find("\r\n\r\n");
    if(pos == std::string::npos)
        return;
    std::string hewders_part = raw_request.substr(0, pos);
    std::istringstream s(hewders_part);
    std::string line;
    std::getline(s, line);
    parse_request_line(line);
    //HADA A si mohssin header
    while(getline(s, line))
    {
        if(line == "\r" || line.empty())
            break;
        size_t colon = line.find(':');
        if(colon == std::string::npos)
            continue;
        std::string key = Utils::trim(line.substr(0, colon));
        std::string value = Utils::trim(line.substr(colon + 1));
        request.headers[key] = value;
    }
    headers_parsed = true;
    // hada parse dyal body
    

}

void Client::parse_request_line(const std::string &line)
{
    std::string trimmed = Utils::trim(line);
    std::vector<std::string> parts = Utils::split(trimmed);

    if(parts.size() < 3)
        return;
    request.method = Utils::to_upper(parts[0]);
    std::string path_and_query = parts[1];
    request.version = parts[3];
    size_t pos_query = path_and_query.find('?');
    if(pos_query != std::string::npos)
    {
        request.path = path_and_query.substr(0, pos_query);
        request.query = path_and_query.substr(pos_query + 1);
    }
    else
    {
        request.path = path_and_query;
        request.query = "";
    }
    if(request.version != "HTTP\1.0" && request.version != "HTTP\1.1")
        return;
}