#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>

Parser::Parser()  {}
Parser::~Parser() {}


std::vector<ServerConfig> Parser::parse(const std::string& configFile)
{
    std::string source = read_file(configFile);
    if (source.empty())
        throw ParserException("failed to read config file: " + configFile);

    TokenList tokens = Lexer::tokenize(source);
    validate_global_scope(tokens);

    std::vector<ServerConfig> configs;
    size_t pos = 0;

    while (peek(tokens, pos).type != TOKEN_EOF)
    {
        const Token& kw = consume(tokens, pos, TOKEN_KEYWORD, "expected 'server'");
        if (kw.value != "server")
            throw ParserException("expected 'server', got '" + kw.value + "'", kw.line);

        ServerConfig cfg = parse_server_block(tokens, pos);
        if (cfg.port == 0)
            throw ParserException("empty or invalid server block");

        configs.push_back(cfg);
    }

    check_duplicate_servers(configs);
    return configs;
}

const Token& Parser::peek(const TokenList& tokens, size_t pos)
{
    if (pos < tokens.size()) 
        return tokens[pos];
    return tokens.back();
}

const Token& Parser::consume(const TokenList& tokens, size_t& pos, TokenType expected, const std::string& context)
{
    const Token& tok = peek(tokens, pos);

    if (tok.type != expected)
    {
        std::string got;

        if (tok.value.empty())
            got = "<punctuation>";
        else
            got = tok.value;

        throw ParserException("unexpected token '" + got + "': " + context, tok.line
        );
    }
    else
        return tokens[pos++];
}

const Token& Parser::consumeAny(const TokenList& tokens, size_t& pos, const std::string& context)
{
    const Token& tok = peek(tokens, pos);

    if (tok.type == TOKEN_EOF ||
        tok.type == TOKEN_LBRACE ||
        tok.type == TOKEN_RBRACE ||
        tok.type == TOKEN_SEMICOLON)
    {
        std::string got;
        if (tok.value.empty())
            got = "<end>";
        else
            got = tok.value;
        throw ParserException("expected a value but got '" + got + "': " + context, tok.line);
    }
    else
    {
        return tokens[pos++];
    }
}

std::vector<std::string> Parser::readValues(const TokenList& tokens, size_t& pos)
{
    std::vector<std::string> values;
    int line = peek(tokens, pos).line;

    while (true)
    {
        const Token& t = peek(tokens, pos);
        if (t.type == TOKEN_SEMICOLON) 
        { 
            pos++; 
            break;
        }
        if (t.type == TOKEN_EOF || t.type == TOKEN_LBRACE || t.type == TOKEN_RBRACE)
            throw ParserException("missing ';'", line);
        values.push_back(t.value);
        pos++;
    }
    return values;
}


void Parser::validate_global_scope(const TokenList& tokens)
{
    size_t pos          = 0;
    int    depth        = 0;
    bool   expect_brace = false;

    while (pos < tokens.size())
    {
        const Token& t = tokens[pos];
        if (t.type == TOKEN_EOF) 
            break;
        if (depth == 0)
        {
            if (expect_brace)
            {
                if (t.type != TOKEN_LBRACE)
                    throw ParserException("expected '{' after 'server'", t.line);
                expect_brace = false;
            }
            else if (t.type == TOKEN_KEYWORD && t.value == "server")
                expect_brace = true;
            else
                throw ParserException("unexpected token '" + t.value + "' in global scope", t.line);
        }
        if (t.type == TOKEN_LBRACE)
            depth++;
        if (t.type == TOKEN_RBRACE)
        {
            depth--;
            if (depth < 0)
                throw ParserException("unexpected '}'", t.line);
        }
        pos++;
    }
    if (depth != 0)
        throw ParserException("unclosed '{' in config file");
}

static std::string int_to_str(int n)
{
    std::ostringstream oss; oss << n; 
        return oss.str();
}

void Parser::check_duplicate_servers(const std::vector<ServerConfig>& configs)
{
    for (size_t i = 0; i < configs.size(); i++)
    {
        for (size_t j = i + 1; j < configs.size(); j++)
        {
            if (configs[i].port != configs[j].port || configs[i].host != configs[j].host)
                continue;

            if (configs[i].server_names.empty() && configs[j].server_names.empty())
                throw ParserException("duplicate host:port with no server_name: " + configs[i].host + ":" + int_to_str(configs[i].port));

            for (size_t si = 0; si < configs[i].server_names.size(); si++)
                for (size_t sj = 0; sj < configs[j].server_names.size(); sj++)
                    if (configs[i].server_names[si] == configs[j].server_names[sj])
                        throw ParserException("duplicate server_name '" + configs[i].server_names[si] + "' on " + configs[i].host + ":" + int_to_str(configs[i].port));
        }
    }
}

std::string Parser::read_file(const std::string& path)
{
    std::ifstream file(path.c_str());
    if (!file.is_open()) 
        return "";
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

bool Parser::isNumber(const std::string& str)
{
    if (str.empty()) 
        return false;
    for (size_t i = 0; i < str.size(); i++)
        if (!std::isdigit(str[i])) return false;
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

size_t Parser::parse_size(const std::string& value)
{
    if (value.empty() || !std::isdigit(value[0]))
        throw ParserException("invalid client_max_body_size: '" + value + "'");

    char               unit       = value[value.size() - 1];
    unsigned long long multiplier = 1;
    std::string        num_part   = value;

    if (std::isalpha(unit))
    {
        num_part = value.substr(0, value.size() - 1);
        if      (unit == 'K' || unit == 'k') multiplier = 1024ULL;
        else if (unit == 'M' || unit == 'm') multiplier = 1024ULL * 1024;
        else if (unit == 'G' || unit == 'g') multiplier = 1024ULL * 1024 * 1024;
        else
            throw ParserException("unknown size unit '" + std::string(1, unit) + "' (use K, M or G)");
    }

    unsigned long long size = std::strtoull(num_part.c_str(), NULL, 10);
    const unsigned long long MAX = 1024ULL * 1024 * 1024;

    if (size > MAX / multiplier)
        throw ParserException("client_max_body_size overflow");

    unsigned long long final_size = size * multiplier;
    if (final_size > MAX)
        throw ParserException("client_max_body_size exceeds maximum (1G)");

    return static_cast<size_t>(final_size);
}