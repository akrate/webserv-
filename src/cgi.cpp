#include "../include/response.hpp"

void freeEnv(char **env)
{
    if (env == NULL)
        return;

    for (int i = 0; env[i] != NULL; i++)
    {
        delete[] env[i];
    }
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
    size_t pos = cgi_output.find("\r\n\r\n");
    size_t pos_len = 4;
    if(pos == std::string::npos)
    {
        pos = cgi_output.find("\n\n");
        pos_len = 2;
    }
    if(pos == std::string::npos)
    {
        res.setStatusCode(200);
        res.headers["content-type"] = "text/html";
        res.setBody(cgi_output);
        return;
    }
    std::string header = cgi_output.substr(0, pos);
    std::string body = cgi_output.substr(pos + pos_len);
    std::istringstream ss(header);
    std::string line;
    while(std::getline(ss, line))
    {
        if(line.empty())
            continue;;
        line = Utils::trim(line);
        size_t colon = line.find(':');
        if(colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        key = Utils::trim(key);
        value = Utils::trim(value);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        if(key == "status")
            res.setStatusCode(atoi(value.c_str()));
        else
            res.headers[key] = value;
    }
    res.setBody(body);
}
char **prepareEnv(const HttpRequest& req,const std::string& scriotpath)
{
    std::vector<std::string> envs;

    envs.push_back("REQUEST_METHOD=" + req.method);
    envs.push_back("QUERY_STRING=" + req.query);
    envs.push_back("SCRIPT_NAME=" + scriotpath);
    envs.push_back("PATH_INFO=" + scriotpath);
    envs.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envs.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envs.push_back("SERVER_SOFTWARE=Webserv/1.0");
    envs.push_back("REDIRECT_STATUS=200");
    if (req.method == "POST")
    {
        std::stringstream ss;
        ss << req.body.size();
        envs.push_back("CONTENT_LENGTH=" + ss.str());
        std::map<std::string, std::string>::const_iterator it = req.headers.find("content-type");
        if (it != req.headers.end())
        {
            envs.push_back("CONTENT_TYPE=" + it->second);
        }
    }
    char **envp = new char*[envs.size() + 1];
    for (size_t i = 0;i < envs.size();i++)
    {
        envp[i] = new char[envs[i].size() + 1];
        std::strcpy(envp[i],envs[i].c_str());
    }
    envp[envs.size()] = NULL;
    return envp;
}
Response Response::execute_cgi(const HttpRequest& req, 
                               const std::string& path, 
                               const LocationConfig& location) 
{
    Response res;
    int pipe_in[2];
    int pipe_out[2];

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1)
        return build_page_error(500);
    char **env = prepareEnv(req, path);
    pid_t pid = fork();
    if (pid == -1)
    {
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        freeEnv(env);
        return build_page_error(500);
    }
    if (pid == 0)
    {
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);

        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);

        char *args[] = {
            (char *)location.cgi_path.c_str(),
            (char *)path.c_str(),
            NULL
        };
        execve(args[0], args, env);
        exit(1);
    } 
    else
    {
        close(pipe_in[0]);
        close(pipe_out[1]);

        if (req.method == "POST")
            write(pipe_in[1], req.body.c_str(), req.body.size());
        close(pipe_in[1]);
        char buffer[4096];
        std::string cgi_output;
        int bytes_read;
        while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
            cgi_output.append(buffer, bytes_read);
        
        close(pipe_out[0]);
        int status;
        waitpid(pid, &status, 0);
        freeEnv(env);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            std::cerr << "CGI Error: Child exited with code " << WEXITSTATUS(status) << std::endl;
            return build_page_error(502);
        }
        else if (WIFSIGNALED(status)) {
            return build_page_error(500);
        }
        parseCgiOutput(cgi_output, res);
    }
    return res;
}