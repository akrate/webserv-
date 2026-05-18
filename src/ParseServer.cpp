#include "Parser.hpp"
#include <cstdlib>

ServerConfig::ServerConfig()
    :   port(0),
        host("0.0.0.0"),
        redirect_code(0),
        client_max_body_size(1024 * 1024)
{}

ServerConfig Parser::parse_server_block(const TokenList& tokens, size_t& pos)
{
    ServerConfig server;
    consume(tokens, pos, TOKEN_LBRACE, "expected '{' to open server block");

    while (true)
    {
        const Token& t = peek(tokens, pos);

        if (t.type == TOKEN_RBRACE) 
        { 
            pos++; 
            break; 
        }
        if (t.type == TOKEN_EOF)
            throw ParserException("unexpected end of file inside server block", t.line);

        if (t.type != TOKEN_KEYWORD)
            throw ParserException("expected directive, got '" + t.value + "'", t.line);

        std::string key = t.value;
        pos++;

        if (key == "location")
        {
            const Token& path_tok = consumeAny(tokens, pos, "'location' requires a path");
            std::string path = path_tok.value;
            if (path.empty() || path[0] != '/')
                throw ParserException("location path must start with '/': '" + path + "'", path_tok.line);
            LocationConfig loc = parse_location_block(tokens, pos, path);
            server.locations.push_back(loc);
            continue;
        }

        std::vector<std::string> vals = readValues(tokens, pos);

        if (key == "listen")
        {
            if(server.port != 0)
                throw ParserException("duplicate port");
            if (vals.size() != 1)
                throw ParserException("'listen' requires exactly one argument", t.line);
            char* end_ptr;
            long port = std::strtol(vals[0].c_str(), &end_ptr, 10);
            if (*end_ptr)
                throw ParserException("invalid port '" + vals[0] + "'", t.line);
            if (port < 1 || port > 65535)
                throw ParserException("port out of range: " + vals[0], t.line);
            server.port = static_cast<int>(port);
        }
        else if (key == "host")
        {
            if (vals.size() != 1)
                throw ParserException("'host' requires exactly one argument", t.line);
            server.host = vals[0];
        }
        else if (key == "server_name")
        {
            if (vals.empty())
                throw ParserException("'server_name' requires at least one argument", t.line);
            for (size_t i = 0; i < vals.size(); i++)
                server.server_names.push_back(vals[i]);
        }
        else if (key == "root")
        {
            if (vals.size() != 1)
                throw ParserException("'root' requires exactly one argument", t.line);
            server.root = vals[0];
        }
        else if (key == "index")
        {
            if (vals.empty())
                throw ParserException("'index' requires at least one argument", t.line);
            for (size_t i = 0; i < vals.size(); i++)
                server.index.push_back(vals[i]);
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
                server.error_pages[code] = uri;
            }
        }
        else if (key == "client_max_body_size")
        {
            if (vals.size() != 1)
                throw ParserException("'client_max_body_size' requires exactly one argument", t.line);
            server.client_max_body_size = parse_size(vals[0]);
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
                    server.redirect_code = parseReturnCode(vals[0]);
                }
                else
                {
                    server.redirect_code = 302;
                    size_t pos = vals[0].find("http://");
                    if(pos == std::string::npos)
                        throw ParserException("invalid return code");
                    server.redirect_url = vals[0];
                }
            }
            else
            {
                server.redirect_code = parseReturnCode(vals[0]);
                server.redirect_url = vals[1];
            }
        }
        else
        {
            throw ParserException("unknown directive '" + key + "' in server block", t.line);
        }
    }

    return server;
}