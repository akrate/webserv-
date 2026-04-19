#include "webserv.hpp"
namespace Utils
{
    std::string trim(const std::string& str)
    {
        size_t start = str.find_first_not_of(" \n\t");
        if(start == std::string::npos)
            return "";
        size_t end = str.find_last_not_of(" \n\t");

        return (str.substr(start, end - start + 1));
    }
    std::vector<std::string> split(const std::string& str)
    {
        std::stringstream ss(str);
        std::vector<std::string> tokens;
        std::string token;

        while(ss >> token)
            tokens.push_back(token);
        return tokens;
    }
    std::string to_upper(const std::string& str)
    {
        std::string s = str;
        for(size_t i = 0; i < s.length(); i++)
            s[i] = std::toupper(s[i]);
        return s;
    }
    
}
  

     