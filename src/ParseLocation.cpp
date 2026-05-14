#include "Parser.hpp"
#include <cstdlib>

LocationConfig::LocationConfig()
    : autoindex(false),
      redirect_code(0),
      allow_upload(false),
      client_max_body_size(0)
{}

LocationConfig Parser::parse_location_block(const TokenList& tokens, size_t& pos, const std::string& path)
{
    LocationConfig location;
    location.path = path;

    consume(tokens, pos, TOKEN_LBRACE, "expected '{' to open location block");
    while (true)
    {
        const Token& t = peek(tokens, pos);
        if (t.type == TOKEN_RBRACE)
        {
            pos++;
            break;
        }
        if (t.type == TOKEN_EOF)
            throw ParserException("unexpected end of file inside location block", t.line);
        if (t.type != TOKEN_KEYWORD)
            throw ParserException("expected directive, got '" + t.value + "'", t.line);
        std::string key = t.value;
        pos++;
        std::vector<std::string> vals = readValues(tokens, pos);
        if (key == "root")
        {
            if (vals.size() != 1)
                throw ParserException("'root' requires exactly one argument", t.line);
            location.root = vals[0];
        }
        else if (key == "index")
        {
            if (vals.empty())
                throw ParserException("'index' requires at least one argument", t.line);
            for (size_t i = 0; i < vals.size(); i++)
                location.index.push_back(vals[i]);
        }
        else if (key == "methods")
        {
            if (vals.empty())
                throw ParserException("'methods' requires at least one argument", t.line);
            const std::string valid[] = {"GET", "POST", "DELETE"};
            for (size_t i = 0; i < vals.size(); i++)
            {
                bool ok = false;
                for (int j = 0; j < 3; j++)
                {
                    if (vals[i] == valid[j])
                    {
                        ok = true;
                        break;
                    }
                }
                if (!ok)
                    throw ParserException("unsupported HTTP method '" + vals[i] + "' (allowed: GET, POST, DELETE)", t.line);
                location.allowed_methods.push_back(vals[i]);
            }
        }
        else if (key == "autoindex")
        {
            if (vals.size() != 1)
                throw ParserException("'autoindex' requires exactly one argument", t.line);
            if (vals[0] != "on" && vals[0] != "off")
                throw ParserException("'autoindex' value must be 'on' or 'off'", t.line);
            location.autoindex = (vals[0] == "on");
        }
        else if (key == "cgi_ext")
        {
            if (vals.empty())
                throw ParserException("'cgi_ext' requires at least one argument", t.line);
            for (size_t i = 0; i < vals.size(); i++)
                location.cgi_extensions.push_back(vals[i]);
        }
        else if (key == "cgi_path")
        {
            if (vals.size() != 1)
                throw ParserException("'cgi_path' requires exactly one argument", t.line);
            location.cgi_path = vals[0];
        }
        else if (key == "upload_store")
        {
            if (vals.size() != 1)
                throw ParserException("'upload_store' requires exactly one argument", t.line);
            location.upload_store = vals[0];
        }
        else if (key == "allow_upload")
        {
            if (vals.size() != 1)
                throw ParserException("'allow_upload' requires exactly one argument", t.line);
            if (vals[0] != "on" && vals[0] != "off")
                throw ParserException("'allow_upload' value must be 'on' or 'off'", t.line);
            location.allow_upload = (vals[0] == "on");
        }
        else if (key == "error_page")
        {
            if (vals.size() < 2)
                throw ParserException("'error_page' requires at least one code and a URI", t.line);
            std::string uri = vals.back();
            for (size_t i = 0; i < vals.size() - 1; i++)
            {
                if (!isNumber(vals[i]))
                    throw ParserException("invalid error code '" + vals[i] + "'", t.line);
                int code = std::atoi(vals[i].c_str());
                if (code < 100 || code > 599)
                    throw ParserException("error code out of range: " + vals[i], t.line);
                location.error_pages[code] = uri;
            }
        }
        else if (key == "client_max_body_size")
        {
            if (vals.size() != 1)
                throw ParserException("'client_max_body_size' requires exactly one argument", t.line);
            location.client_max_body_size = parse_size(vals[0]);
        }
        else if (key == "return")
        {
            if (vals.empty())
                throw ParserException("'return' requires at least one argument", t.line);
            if (vals.size() > 2)
                throw ParserException("too many arguments for 'return'", t.line);
            if (vals.size() == 1)
            {
                if (isNumber(vals[0]))
                {
                    location.redirect_code = parseReturnCode(vals[0]);
                }
                else
                {
                    location.redirect_code = 302;
                    location.redirect_url = vals[0];
                }
            }
            else
            {
                location.redirect_code = parseReturnCode(vals[0]);
                location.redirect_url = vals[1];
            }
        }
        else
            throw ParserException("unknown directive '" + key + "' in location block", t.line);
    }
    return location;
}