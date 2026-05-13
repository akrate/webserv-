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
