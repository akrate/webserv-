#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include "ParserException.hpp"

enum TokenType
{
    TOKEN_KEYWORD,   
    TOKEN_STRING,       
    TOKEN_LBRACE,       
    TOKEN_RBRACE,       
    TOKEN_SEMICOLON,    
    TOKEN_EOF
};

struct Token
{
    TokenType   type;
    std::string value;
    int         line;   

    Token(TokenType t, const std::string& v, int l)
        : type(t), value(v), line(l) {}
};

class Lexer
{
public:
    static std::vector<Token> tokenize(const std::string& source);

private:
    static bool isKeyword(const std::string& word);
};

#endif