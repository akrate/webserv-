#include "Parser.hpp"

Parser::Parser() {
}

Parser::~Parser() {
}

std::vector<ServerConfig> Parser::parse(const std::string& configFile)
{
    std::vector<ServerConfig> configs;
    std::string content = read_File(configFile);
    if (content.empty()) {
        std::cerr << "Error: Failed to read config file: " << configFile << std::endl;
        return configs;
    }
    remouve_comments(content);
    validate_brackets(content);
    validate_structure(content);
    validate_semicolons(content);
    size_t pos = 0;
    while(pos < content.size())
    {
        size_t server_pos = content.find("server {", pos);
        if(server_pos == std::string::npos)
            break;
        ServerConfig config = parse_server_block(content, server_pos);
        configs.push_back(config);
        pos = server_pos + 1;
    }
    for(size_t i = 0; i < configs.size(); i++)
        for(size_t j = i + 1; j < configs.size(); j++)
            if(configs[i].port == configs[j].port && configs[i].host == configs[j].host)
            {
                std::cerr << "Error: duplicate port " << configs[i].port << std::endl;
                exit(1);
            }
    return configs;
}
void Parser::validate_semicolons(const std::string& content)
{
    std::istringstream s(content);
    std::string line;
    int line_num = 0;

    while (std::getline(s, line))
    {
        line_num++;
        std::string trimmed = Utils::trim(line);

        if (trimmed.empty()) 
            continue;
        if (trimmed[0] == '}') 
            continue;
        if (trimmed[trimmed.size()-1] == '{')
            continue;
        if (trimmed[trimmed.size()-1] == '}')
            continue;
        if (trimmed[trimmed.size()-1] != ';')
        {
            std::cerr << "Error: missing ';' at line " 
                      << line_num << ": \"" << trimmed << "\"" << std::endl;
            exit(1);
        }
    }
}
void Parser::validate_brackets(const std::string& content)
{
    int count = 0;
    int line = 1;
    for(size_t i = 0; i < content.length(); i++)
    {
        if(content[i] == '\n')
            line++;
        else if(content[i] == '{')
            count++;
        else if(content[i] == '}')
        {
            count--;
            if(count < 0)
            {
                std::cerr << "Error: unexpected '}' at line " << line << std::endl;
                exit(1);
            }
        }
        
    }
    if(count != 0)
    {
        std::cerr << "Error: unclosed '{' in config file" << "\n";
        exit(1);
    }
}
void Parser::validate_structure(const std::string& content)
{
    std::istringstream stream(content);
    std::string line;
    int line_num = 0;

    while(std::getline(stream, line))
    {
        line_num++;
        std::string trimmed = Utils::trim(line);
        if(trimmed.empty()) continue;

        std::string first_word = trimmed.substr(0, trimmed.find_first_of(" \t"));

        if(first_word == "server" || first_word == "location")
        {
            if(trimmed.find('{') == std::string::npos)
            {
                std::cerr << "Error: missing '{' after '"
                          << trimmed << "' at line "
                          << line_num << std::endl;
                exit(1);
            }
        }
    }
}
std::string Parser::read_File(const std::string& path)
{
    std::ifstream file(path.c_str());
    if(!file.is_open())
        return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

void Parser::remouve_comments(std::string& content)
{
    std::string result;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        if (!line.empty() || comment_pos != std::string::npos) {
            result += line + "\n";
        }
    }
    content = result;
}
size_t parse_size(const std::string& value)
{
    if (value.empty()) return 0;

    size_t m = 1;
    char unit = value[value.length() - 1];
    std::string num_part = value;

    if (isalpha(unit))
    {
        num_part = value.substr(0, value.length() - 1);
        if (unit == 'M' || unit == 'm') m = 1024 * 1024;
        else if (unit == 'K' || unit == 'k') m = 1024;
        else if (unit == 'G' || unit == 'g') m = 1024 * 1024 * 1024;
    }

    unsigned long long size = std::strtoull(num_part.c_str(), NULL, 10);
    if (size > 10240 && (m > 1024)) { 
        std::cerr << "Error: client_max_body_size is too large!" << std::endl;
        exit(1);
    }
    return static_cast<size_t>(size * m);
}
ServerConfig Parser::parse_server_block(const std::string& content, size_t &pos)
{
    ServerConfig config;

    size_t start = content.find('{', pos);
    if(start == std::string::npos)
        return config;
    std::string block = extarct_block(content, start);
    pos = start;
    size_t block_pos = 0;
    while (block_pos < block.length())
    {
        size_t end = block.find('\n', block_pos);
        if(end == std::string::npos)
            end = block.length();
        std::string line = Utils::trim(block.substr(block_pos, end - block_pos));
        block_pos = end + 1;
        if(line.empty() || line[0] == '#')
            continue;
        if (!line.empty() && line[line.length() - 1] == ';')
            line.erase(line.length() - 1); 
        std::vector<std::string> parts = Utils::split(line);
        // for(size_t i = 0; i < parts.size(); i++)
        // {
        //     std::cout << "[" << parts[i] << "] ";
        // }
        // std::cout << std::endl;
        if(parts.empty())
            continue;
        std::string key = parts[0];
        if (key == "listen")
        {
            if (parts.size() > 1) {
                long port = std::atol(parts[1].c_str());
                if (port < 1 || port > 65535) {
                    std::cerr << "Error: Invalid port number '" << parts[1] << "' (must be 1-65535)" << std::endl;
                    exit(1);
                }
                config.port = static_cast<int>(port);
            }
        }
        else if(key == "server_name")
        {
            for(size_t i = 1; i < parts.size(); i++)
                config.server_names.push_back(parts[i]);
        }
        else if(key == "root")
        {
            if(parts.size() > 1)
                config.root = parts[1];
        }
        else if (key == "host")
        {
            if(parts.size() > 1)
                config.host = parts[1]; 
        }
        else if(key == "index")
        {
            for(size_t i = 1; i < parts.size(); i++)
                config.index.push_back(parts[i]);
        }
        else if(key == "error_page")
        {
            if(parts.size() > 2)
            {
                int code = atoi(parts[1].c_str());
                config.error_pages[code] = parts[2];
            }
        }
        else if(key == "client_max_body_size")
        {
            if(parts.size() > 1)
            if (parts.size() > 1)
                config.client_max_body_size = parse_size(parts[1]);
            // std::cout << "Max body size: " << config.client_max_body_size << " bytes" << std::endl;
        }
        else if(key == "location")
        {
            std::string path;
            for (size_t i = 1; i < parts.size(); i++)
            {
                if (parts[i] != "{") {
                    path = parts[i];
                    break;
                }
            }
            if (!path.empty())
            {
                size_t line_start = block.rfind('\n', block_pos - 2);
                if (line_start == std::string::npos)
                    line_start = 0;
                else
                    line_start++;
                block_pos = line_start;

                LocationConfig loc = parse_location_block(block, block_pos, path);
                config.locations.push_back(loc);
            }
        }

    }
    pos = start;
    return config;
}

LocationConfig Parser::parse_location_block(const std::string& content, size_t& pos, std::string& path)
{
    LocationConfig location;
    location.path = path;
    location.autoindex = false;
    size_t brace_start = content.find('{', pos);
    if (brace_start == std::string::npos)
        return location;
    std::string block = extarct_block(content, brace_start);
    pos = brace_start;

    size_t block_pos = 0;
    while (block_pos < block.length())
    {
        size_t end = block.find('\n', block_pos);
        if (end == std::string::npos)
            end = block.length();
        std::string line = Utils::trim(block.substr(block_pos, end - block_pos));
        block_pos = end + 1;

        if (line.empty() || line[0] == '#')
            continue;

        if (!line.empty() && line[line.length() - 1] == ';')
            line.erase(line.length() - 1);

        std::vector<std::string> parts = Utils::split(line);
        // for(size_t i = 0; i < parts.size(); i++)
        // {
        //     std::cout << "[" << parts[i] << "] ";
        // }
        // std::cout << std::endl;
        if (parts.empty())
            continue;
        std::string key = parts[0];
        if(key == "root")
        {
            if(parts.size() > 1)
                location.root = parts[1];
        }
        else if(key == "index")
        {
            for(size_t i = 1; i < parts.size(); i++)
                location.index.push_back(parts[i]);
        }
        else if(key == "methods")
        {
            for(size_t i = 1; i < parts.size(); i++)
                location.allowed_methods.push_back(parts[i]);
        }
        else if(key == "autoindex")
        {
            if(parts.size() > 1)
            {
                if(parts[1] == "on")
                    location.autoindex = true;
                else
                    location.autoindex = false;
            }
                
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
        else if(key == "upload_store")
        {
            if(parts.size() > 1)
                location.upload_store = parts[1];
        }
        else if(key == "allow_upload")
        {
            if(parts.size() > 1)
                location.allow_upload = (parts[1] == "on");
        }
        else if (key == "return")
        {
            if (parts.size() > 2)
            {
                int code = std::atoi(parts[1].c_str());
                if (code < 300 || code > 599) {
                    std::cerr << "Error: Invalid return code '" << code << "'" << std::endl;
                    exit(1);
                }
                location.redirect_code = code;
                location.redirect_url  = parts[2];
            }
            else if (parts.size() > 1)
            {
                location.redirect_code = 301;
                location.redirect_url  = parts[1];
            }
        }

    }
    return location;
}

std::string Parser::extarct_block(const std::string& content, size_t& pos)
{
    if(pos >= content.length() || content[pos] != '{')
        return "";
    size_t start = pos + 1;
    int brace_count = 1;
    size_t current = start;

    while(current < content.length() && brace_count > 0)
    {
        if(content[current] == '{')
            brace_count++;
        else if(content[current] == '}')
            brace_count--;
        current++;
    }
    pos = current;
    return content.substr(start, current - start - 1);
}