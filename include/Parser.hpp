#ifndef PARSER_HPP
#define PARSER_HPP

#include "webserv.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include "ParserException.hpp"
#include "Lexer.hpp"
#include <vector>
#include <limits>

class Parser
{
public:
    Parser();
    ~Parser();

    static std::vector<ServerConfig> parse(const std::string& configFile);

private:
    typedef std::vector<Token> TokenList;

    static const Token& peek(const TokenList& tokens, size_t pos);
    static const Token& consume(const TokenList& tokens, size_t& pos, TokenType expected, const std::string& context);
    static const Token& consumeAny(const TokenList& tokens, size_t& pos, const std::string& context);

    static ServerConfig   parse_server_block  (const TokenList& tokens, size_t& pos);
    static LocationConfig parse_location_block(const TokenList& tokens, size_t& pos, const std::string& path);

    static std::vector<std::string> readValues(const TokenList& tokens, size_t& pos);

    static void validate_global_scope (const TokenList& tokens);
    static void check_duplicate_servers(const std::vector<ServerConfig>& configs);

    static std::string read_file      (const std::string& path);
    static bool        isNumber       (const std::string& str);
    static int         parseReturnCode(const std::string& value);
    static size_t      parse_size     (const std::string& value);
};

#endif