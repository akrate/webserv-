#include "../include/Parser.hpp"


LocationConfig::LocationConfig()
    : autoindex(false),
    redirect_code(0),
    allow_upload(false),
    client_max_body_size(0)
{}

LocationConfig Parser::parse_location_block(const std::string& content, size_t& pos, std::string& path)
{
    LocationConfig location;
    location.path      = path;
    location.autoindex = false;

    size_t brace_start = content.find('{', pos);
    if (brace_start == std::string::npos)
        throw ParserException("missing '{' in location block for path '" + path + "'");

    std::string block = extract_block(content, brace_start);
    pos = brace_start;

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

        if (key == "root")
        {
            if (parts.size() < 2)
                throw ParserException("'root' requires an argument");
            location.root = parts[1];
        }
        else if (key == "index")
        {
            if (parts.size() < 2)
                throw ParserException("'index' requires at least one argument");
            for (size_t i = 1; i < parts.size(); i++)
                location.index.push_back(parts[i]);
        }
        else if (key == "methods")
        {
            if (parts.size() < 2)
                throw ParserException("'methods' requires at least one argument");
            const std::string valid[] = {"GET", "POST", "DELETE"};
            for (size_t i = 1; i < parts.size(); i++)
            {
                bool ok = false;
                for (int v = 0; v < 3; v++)
                    if (parts[i] == valid[v]) 
                    { 
                        ok = true; 
                        break; 
                    }
                if (!ok)
                    throw ParserException("unsupported HTTP method '" + parts[i] + "' (allowed: GET, POST, DELETE)");
                location.allowed_methods.push_back(parts[i]);
            }
        }
        else if (key == "autoindex")
        {
            if (parts.size() == 2)
            {
                if (parts[1] != "on" && parts[1] != "off")
                    throw ParserException("'autoindex' value must be 'on' or 'off'");
                location.autoindex = (parts[1] == "on");
            }
            else
                throw ParserException("'autoindex' requires 'on' or 'off'");

        }
        else if(key == "cgi_ext")
        {
            if(parts.size() > 1)
            {
                for(size_t i = 1; i < parts.size(); i++)
                    location.cgi_extensions.push_back(parts[i]);
            }
        }
        else if(key == "cgi_path")
        {
            if(parts.size() > 1)
                location.cgi_path = parts[1];
        }
        else if (key == "upload_store")
        {
            if (parts.size() < 2)
                throw ParserException("'upload_store' requires a path argument");
            location.upload_store = parts[1];
        }
        else if (key == "allow_upload")
        {
            if (parts.size() == 2)
            {
                if (parts[1] != "on" && parts[1] != "off")
                    throw ParserException("'allow_upload' value must be 'on' or 'off'");
                location.allow_upload = (parts[1] == "on");
            }
            else
                throw ParserException("'allow_upload' requires 'on' or 'off'");

        }
        else if (key == "error_page")
        {
            if (parts.size() < 3)
                throw ParserException("'error_page' requires at least one code and a URI");
            std::string uri = parts[parts.size() - 1];
            for (size_t i = 1; i < parts.size() - 1; i++)
            {
                if (!isNumber(parts[i]))
                    throw ParserException("invalid error code '" + parts[i] + "'");
                int code = std::atoi(parts[i].c_str());
                if (code < 100 || code > 599)
                    throw ParserException("error code out of range: " + parts[i]);
                location.error_pages[code] = uri;
            }
        }
        else if (key == "client_max_body_size")
        {
            if (parts.size() != 2)
                throw ParserException("'client_max_body_size' requires exactly one argument");
            location.client_max_body_size = parse_size(parts[1]);
        }
        else if (key == "return")
        {
            if (parts.size() < 2)
                throw ParserException("'return' requires at least one argument");
            if (parts.size() > 3)
                throw ParserException("too many arguments for 'return'");
            if (parts.size() == 2)
            {
                if (isNumber(parts[1]))
                    location.redirect_code = parseReturnCode(parts[1]);
                else
                {
                    location.redirect_code = 302;
                    location.redirect_url  = parts[1];
                }
            }
            else
            {
                location.redirect_code = parseReturnCode(parts[1]);
                location.redirect_url  = parts[2];
            }
        }
        else
            throw ParserException("unknown directive '" + key + "' in location block");

    }
    return location;
}