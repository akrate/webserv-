#include "../include/Parser.hpp"

// bool Parser::isNumber(const std::string& str)
// {
//     if (str.empty())
//         return false;

//     for (size_t i = 0; i < str.length(); i++)
//     {
//         if (!std::isdigit(str[i]))
//             return false;
//     }
//     return true;
// }

// int Parser::parseReturnCode(const std::string& value)
// {
//     if (!isNumber(value))
//         throw ParserException("invalid return code '" + value + "'");
//     int code = std::atoi(value.c_str());
//     if (code < 100 || code > 599)
//         throw ParserException("invalid return code '" + value + "'");
//     return code;
// }
// std::string Parser::extract_block(const std::string& content, size_t& pos)
// {
//     if(pos >= content.length() || content[pos] != '{')
//         return "";
//     size_t start = pos + 1;
//     int brace_count = 1;
//     size_t current = start;

//     while(current < content.length() && brace_count > 0)
//     {
//         if(content[current] == '{')
//             brace_count++;
//         else if(content[current] == '}')
//             brace_count--;
//         current++;
//     }
//     pos = current;
//     return content.substr(start, current - start - 1);
// }

// size_t Parser::parse_size(const std::string& value)
// {
//     if (value.empty() || !isdigit(value[0]))
//         throw ParserException("invalid client_max_body_size value: '" + value + "'");

//     char unit = value[value.length() - 1];
//     unsigned long long multiplier = 1;
//     std::string num_part = value;

//     if (std::isalpha(unit))
//     {
//         num_part = value.substr(0, value.length() - 1);
//         if      (unit == 'K' || unit == 'k') multiplier = 1024ULL;
//         else if (unit == 'M' || unit == 'm') multiplier = 1024ULL * 1024;
//         else if (unit == 'G' || unit == 'g') multiplier = 1024ULL * 1024 * 1024;
//         else
//             throw ParserException("unknown size unit '" + std::string(1, unit)
//                                   + "' (use K, M or G)");
//     }
//     unsigned long long size = std::strtoull(num_part.c_str(), NULL, 10);

//     if (size > std::numeric_limits<unsigned long long>::max() / multiplier)
//         throw ParserException("client_max_body_size overflow");

//     unsigned long long final_size = size * multiplier;

//     const unsigned long long MAX_BODY_SIZE = 1024ULL * 1024 * 1024; // 1 GB
//     if (final_size > MAX_BODY_SIZE)
//         throw ParserException("client_max_body_size exceeds maximum (1G)");

//     return static_cast<size_t>(final_size);
    
// }