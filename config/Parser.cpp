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
    return configs;
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
    size_t pos = 0;
    while ((pos = content.find('#', pos)) != std::string::npos)
    {
        size_t end = content.find('\n', pos);
        if(end == std::string::npos)
            content.erase(pos);
        else
            content.erase(pos, end - pos);
    }
}
ServerConfig Parser::parse_server_block(const std::string& content, size_t &pos)
{
    
}