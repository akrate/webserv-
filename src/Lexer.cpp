#include "Lexer.hpp"
#include <cctype>

static const char* KEYWORDS[] = {
    "server", "location",
    "listen", "host", "server_name",
    "root", "index", "methods",
    "autoindex", "return",
    "error_page",
    "client_max_body_size",
    "cgi_ext", "cgi_path",
    "upload_store", "allow_upload",
    NULL
};

bool Lexer::isKeyword(const std::string& word)
{
    for (int i = 0; KEYWORDS[i] != NULL; i++)
    {
        if (word == KEYWORDS[i])
            return true;
    }
    return false;
}

std::vector<Token> Lexer::tokenize(const std::string& source)
{
    std::vector<Token> tokens;
    size_t i    = 0;
    int    line = 1;

    while (i < source.size())
    {
        char c = source[i];
        if (c == '\n') 
        { 
            line++; 
            i++; 
            continue; 
        }
        if (std::isspace(c))
        { 
            i++;
            continue; 
        }
        if (c == '#')
        {
            while (i < source.size() && source[i] != '\n')
                i++;
            continue;
        }
        if (c == '{') 
        { 
            tokens.push_back(Token(TOKEN_LBRACE,    "{", line)); 
            i++; 
            continue; 
        }
        if (c == '}') 
        { 
            tokens.push_back(Token(TOKEN_RBRACE,    "}", line)); 
            i++; 
            continue; 
        }
        if (c == ';') 
        { 
            tokens.push_back(Token(TOKEN_SEMICOLON, ";", line)); 
            i++; 
            continue; 
        }
        if (c == '"' || c == '\'')
        {
            char quote = c;
            i++;
            std::string val;
            while (i < source.size() && source[i] != quote)
            {
                if (source[i] == '\n') 
                    line++;
                val += source[i++];
            }
            if (i >= source.size())
                throw ParserException("unterminated string literal", line);
            i++; 
            tokens.push_back(Token(TOKEN_STRING, val, line));
            continue;
        }

        if (!std::isspace(c) && c != '{' && c != '}' && c != ';' && c != '#')
        {
            std::string word;
            while (i < source.size() && !std::isspace(source[i]) && source[i] != '{' && source[i] != '}' &&
                   source[i] != ';' && source[i] != '#')
            {
                word += source[i++];
            }
            TokenType t;
            if(isKeyword(word))
                t = TOKEN_KEYWORD;
            else
                t = TOKEN_STRING;
            tokens.push_back(Token(t, word, line));
            continue;
        }
        i++;
    }

    tokens.push_back(Token(TOKEN_EOF, "", line));
    return tokens;
}