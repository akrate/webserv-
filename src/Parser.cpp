#include "../include/Parser.hpp"

Parser::Parser() {
}

Parser::~Parser() {
}

static std::string to_str(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}


std::vector<ServerConfig> Parser::parse(const std::string& configFile)
{
    std::vector<ServerConfig> configs;
    std::string content = read_file(configFile);
    if (content.empty())
        throw ParserException("failed to read config file: " + configFile);

    remove_comments(content);
    validate_brackets(content);
    validate_structure(content);
    validate_global_scope(content);
    validate_semicolons(content);

    size_t pos = 0;
    while (pos < content.size())
    {
        size_t server_pos = content.find("server {", pos);
        if (server_pos == std::string::npos)
            break;
        ServerConfig config = parse_server_block(content, server_pos);
        configs.push_back(config);
        pos = server_pos + 1;
    }
    for (size_t i = 0; i < configs.size(); i++)
    {
        for (size_t j = i + 1; j < configs.size(); j++)
        {
            if (configs[i].port != configs[j].port || configs[i].host != configs[j].host)
                continue;

            if (configs[i].server_names.empty() && configs[j].server_names.empty())
                throw ParserException("duplicate host:port with no server_name: " + configs[i].host + ":" + to_str(configs[i].port));

            for (size_t si = 0; si < configs[i].server_names.size(); si++)
                for (size_t sj = 0; sj < configs[j].server_names.size(); sj++)
                    if (configs[i].server_names[si] == configs[j].server_names[sj])
                        throw ParserException("duplicate server_name '" + configs[i].server_names[si] + "' on " + configs[i].host + ":" + to_str(configs[i].port));
        }
    }
    return configs;
}
void Parser::validate_global_scope(const std::string& content)
{
    size_t pos = 0;

    while (pos < content.size())
    {
        while (pos < content.size() && std::isspace(content[pos]))
            pos++;

        if (pos >= content.size())
            break;
        if (content.compare(pos, 6, "server") != 0)
            throw ParserException("invalid directive in global scope");
        size_t brace = content.find('{', pos);
        if (brace == std::string::npos)
            throw ParserException("missing '{' after server");
        int brackets = 1;
        pos = brace + 1;
        while (pos < content.size() && brackets > 0)
        {
            if (content[pos] == '{')
                brackets++;
            else if (content[pos] == '}')
                brackets--;

            pos++;
        }
        if (brackets != 0)
            throw ParserException("unclosed server block");
    }
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
            throw ParserException("missing ';' -> \"" + trimmed + "\"", line_num);
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
                throw ParserException("unexpected '}'", line);
        }
        
    }
    if(count != 0)
        throw ParserException("unclosed '{' in config file");
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
                throw ParserException("missing '{' after '" + trimmed + "'", line_num);
        }
    }
}
std::string Parser::read_file(const std::string& path)
{
    std::ifstream file(path.c_str());
    if(!file.is_open())
        return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

void Parser::remove_comments(std::string& content)
{
    std::string result;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) 
    {
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
    if (value.empty() || !isdigit(value[0]))
        throw ParserException("invalid client_max_body_size value: '" + value + "'");

    char unit = value[value.length() - 1];
    unsigned long long multiplier = 1;
    std::string num_part = value;

    if (std::isalpha(unit))
    {
        num_part = value.substr(0, value.length() - 1);
        if      (unit == 'K' || unit == 'k') multiplier = 1024ULL;
        else if (unit == 'M' || unit == 'm') multiplier = 1024ULL * 1024;
        else if (unit == 'G' || unit == 'g') multiplier = 1024ULL * 1024 * 1024;
        else
            throw ParserException("unknown size unit '" + std::string(1, unit)
                                  + "' (use K, M or G)");
    }
    unsigned long long size = std::strtoull(num_part.c_str(), NULL, 10);

    if (size > std::numeric_limits<unsigned long long>::max() / multiplier)
        throw ParserException("client_max_body_size overflow");

    unsigned long long final_size = size * multiplier;

    const unsigned long long MAX_BODY_SIZE = 1024ULL * 1024 * 1024; // 1 GB
    if (final_size > MAX_BODY_SIZE)
        throw ParserException("client_max_body_size exceeds maximum (1G)");

    return static_cast<size_t>(final_size);
}

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

bool Parser::isNumber(const std::string& str)
{
    if (str.empty())
        return false;

    for (size_t i = 0; i < str.length(); i++)
    {
        if (!std::isdigit(str[i]))
            return false;
    }
    return true;
}

int Parser::parseReturnCode(const std::string& value)
{
    if (!isNumber(value))
        throw ParserException("invalid return code '" + value + "'");
    int code = std::atoi(value.c_str());
    if (code < 100 || code > 599)
        throw ParserException("invalid return code '" + value + "'");
    return code;
}
std::string Parser::extract_block(const std::string& content, size_t& pos)
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