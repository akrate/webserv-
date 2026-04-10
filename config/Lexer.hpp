#ifndef LEXER_HPP   
#define LEXER_HPP

#include <vector>
#include <string>

class Lexer {
public:
    Lexer(const std::string &input);
    std::vector<Token> tokenize();
private:
    std::string input;
    size_t position;
};

#endif