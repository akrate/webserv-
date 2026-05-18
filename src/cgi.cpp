#include "../include/response.hpp"

void freeEnv(char **env)
{
    if (env == NULL)
        return;
    for (int i = 0; env[i] != NULL; i++)
        delete[] env[i];
    delete[] env;
}

bool isCgi(const std::string& path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return false;
    std::string ext = path.substr(dot);
    return (ext == ".py" || ext == ".cgi" || ext == ".php");
}

void Response::parseCgiOutput(const std::string& cgi_output, Response& res)
{
    size_t pos     = cgi_output.find("\r\n\r\n");
    size_t pos_len = 4;

    if (pos == std::string::npos)
    {
        pos     = cgi_output.find("\n\n");
        pos_len = 2;
    }
    if (pos == std::string::npos)
    {
        res.setStatusCode(200);
        res.headers["content-type"] = "text/html";
        res.setBody(cgi_output);
        return;
    }
    std::string header_part = cgi_output.substr(0, pos);
    std::string body        = cgi_output.substr(pos + pos_len);

    std::istringstream ss(header_part);
    std::string line;
    while (std::getline(ss, line))
    {
        if (line.empty()) continue;

        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key   = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        size_t s = key.find_first_not_of(" \t");
        if (s != std::string::npos) key = key.substr(s);
        s = value.find_first_not_of(" \t");
        if (s != std::string::npos) value = value.substr(s);

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        if (key == "status")
            res.setStatusCode(atoi(value.c_str()));
        else
            res.headers[key] = value;
    }

    if (res.status_code == 0)
        res.setStatusCode(200);

    res.setBody(body);
}

char **prepareEnv(const HttpRequest& req, const std::string& scriptPath)
{
    std::vector<std::string> envs;

    envs.push_back("REQUEST_METHOD=" + req.method);
    envs.push_back("QUERY_STRING=" + req.query);
    envs.push_back("SCRIPT_NAME=" + scriptPath);
    envs.push_back("PATH_INFO=" + scriptPath);
    envs.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envs.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envs.push_back("SERVER_SOFTWARE=Webserv/1.0");
    envs.push_back("REDIRECT_STATUS=200");

    if (req.method == "POST")
    {
        std::stringstream ss;
        ss << req.body.size();
        envs.push_back("CONTENT_LENGTH=" + ss.str());
        std::map<std::string, std::string>::const_iterator it =
            req.headers.find("content-type");
        if (it != req.headers.end())
            envs.push_back("CONTENT_TYPE=" + it->second);
    }

    char **envp = new char*[envs.size() + 1];
    for (size_t i = 0; i < envs.size(); i++)
    {
        envp[i] = new char[envs[i].size() + 1];
        std::strcpy(envp[i], envs[i].c_str());
    }
    envp[envs.size()] = NULL;
    return envp;
}
