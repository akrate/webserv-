#ifndef PARSEREXCEPTION_HPP
#define PARSEREXCEPTION_HPP
#include <stdexcept>
#include <sstream>
#include <string>

class ParserException : public std::runtime_error {
public:
    explicit ParserException(const std::string& msg)
        : std::runtime_error("Parser Error: " + msg) {}

    ParserException(const std::string& msg, int line)
        : std::runtime_error(build(msg, line)) {}

private:
    static std::string build(const std::string& msg, int line)
    {
        std::ostringstream oss;
        oss << "Parser Error at line " << line << ": " << msg;
        return oss.str();
    }
};

#endif