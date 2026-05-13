#include "../include/Parser.hpp"

ServerConfig::ServerConfig()
    : port(0),
    host("0.0.0.0"),
    client_max_body_size(1024 * 1024)
{}

ServerConfig Parser::parse_server_block(const std::string& content, size_t& pos)
{
    ServerConfig config;
    config.host = "0.0.0.0";
    config.port = 80;

    size_t start = content.find('{', pos);
    if (start == std::string::npos)
        throw ParserException("missing '{' in server block");

    std::string block = extract_block(content, start);
    pos = start;

    size_t block_pos = 0;
    while (block_pos < block.length())
    {
        size_t end = block.find('\n', block_pos);
        if (end == std::string::npos) end = block.length();

        std::string line = Utils::trim(block.substr(block_pos, end - block_pos));
        block_pos = end + 1;

        if (line.empty() || line[0] == '#') 
            continue;
        if (!line.empty() && line[line.length() - 1] == ';')
            line.erase(line.length() - 1);

        std::vector<std::string> parts = Utils::split(line);
        if (parts.empty()) 
            continue;

        std::string key = parts[0];

        if (key == "listen")
        {
            if (parts.size() == 2) {
                char *end;
                long port = std::strtol(parts[1].c_str(), &end, 10);
                if(*end)
                    throw ParserException("invalid port");
                if (port < 1 || port > 65535)
                    throw ParserException("invalid port number '" + parts[1] + "' (must be 1-65535)");
                config.port = static_cast<int>(port);
            }
            if(parts.size() > 2)
                throw ParserException("too many arguments for 'listen'");
        }
        else if (key == "host")
        {
            if (parts.size() < 2)
                throw ParserException("'host' requires an argument");
            if (parts.size() > 2)
                throw ParserException("too many arguments for 'host'");
            config.host = parts[1];
        }
        else if (key == "server_name")
        {
            if (parts.size() < 2)
                throw ParserException("'server_name' requires at least one argument");
            for (size_t i = 1; i < parts.size(); i++)
                config.server_names.push_back(parts[i]);
        }
        else if (key == "root")
        {
            if (parts.size() < 2)
                throw ParserException("'root' requires an argument");
            config.root = parts[1];
        }
        else if (key == "index")
        {
            if (parts.size() < 2)
                throw ParserException("'index' requires at least one argument");
            for (size_t i = 1; i < parts.size(); i++)
                config.index.push_back(parts[i]);
        }
        else if (key == "error_page")
        {
            if (parts.size() < 3)
                throw ParserException("'error_page' requires at least one code and a URI");
            std::string uri = parts[parts.size() - 1];
            if (uri.empty() || uri[0] != '/')//oussama
            {
                throw ParserException("invalid URI '" + uri + "' in error_page. Must start with '/'");
            }
            for (size_t i = 1; i < parts.size() - 1; i++)
            {
                if (!isNumber(parts[i]))
                    throw ParserException("invalid error code '" + parts[i] + "'");
                int code = std::atoi(parts[i].c_str());
                if (code < 100 || code > 599)
                    throw ParserException("error code out of range: " + parts[i]);
                config.error_pages[code] = uri;
            }
        }
        else if (key == "client_max_body_size")
        {
            if (parts.size() != 2)
                throw ParserException("'client_max_body_size' requires exactly one argument");
            config.client_max_body_size = parse_size(parts[1]);
        }
        else if (key == "location")
        {
            std::string path;
            size_t path_count = 0;
            for (size_t i = 1; i < parts.size(); i++)
            {
                if (parts[i] != "{") 
                { 
                    path = parts[i]; 
                    path_count++; 
                }
            }
            if(path_count != 1)
                throw ParserException("location requires exactly one path");
            if (path.empty())
                throw ParserException("'location' requires a path argument");
            if (path[0] != '/')
                throw ParserException("location path must start with '/': '" + path + "'");

            size_t line_start = block.rfind('\n', block_pos - 2);
            if (line_start == std::string::npos)
                line_start = 0;
            else
                line_start = line_start + 1;
            block_pos = line_start;

            LocationConfig loc = parse_location_block(block, block_pos, path);
            config.locations.push_back(loc);
        }
        else
            throw ParserException("unknown directive '" + key + "' in server block");

    }
    pos = start;
    return config;
}