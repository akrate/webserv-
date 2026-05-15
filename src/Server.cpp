#include "Server.hpp"
#include "client.hpp"
#include "../include/response.hpp"

Server::Server() {}

Server::~Server() {
    close_all_sockets();
}

void Server::close_all_sockets() {
    for (size_t i = 0; i < listen_fds.size(); i++)
        if (listen_fds[i] >= 0)
            close(listen_fds[i]);
    listen_fds.clear();
}

void Server::disconnect_client(std::vector<struct pollfd>& fds, size_t i)
{
    int fd = fds[i].fd;
    close(fd);
    clients.erase(fd);
    client_config_index.erase(fd);
    client_last_active.erase(fd);
    fds.erase(fds.begin() + i);
    std::cout << "Client disconnected (fd=" << fd << ")" << std::endl;
}

void Server::check_timeouts(std::vector<struct pollfd>& fds)
{
    time_t now = time(NULL);
    for (size_t i = fds.size(); i-- > 0;)
    {
        int fd = fds[i].fd;
        if (is_listen_fd(fd))
            continue;
        if (cgi_processes.count(fd))
            continue;
        if (now - client_last_active[fd] >= CLIENT_TIMEOUT)
        {
            std::cout << "Client timeout (fd=" << fd << ")" << std::endl;
            std::string msg =
                "HTTP/1.1 408 Request Timeout\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n";
            send(fd, msg.c_str(), msg.size(), 0);
            disconnect_client(fds, i);
        }
    }
}

int Server::create_socket(const ServerConfig& config)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "Error: socket() failed for port "
                  << config.port << ": " << strerror(errno) << std::endl;
        return -1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: setsockopt() failed for port "
                  << config.port << ": " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error: fcntl() failed for port "
                  << config.port << ": " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(config.port);

    if (inet_pton(AF_INET, config.host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Error: invalid host address '" << config.host << "'" << std::endl;
        close(fd);
        return -1;
    }

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error: bind() failed on " << config.host
                  << ":" << config.port << " - " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    if (listen(fd, 128) < 0) {
        std::cerr << "Error: listen() failed on port "
                  << config.port << ": " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    std::cout << "Listening on " << config.host << ":" << config.port
              << " (fd=" << fd << ")" << std::endl;
    return fd;
}

int Server::init(const std::string& configFile)
{
    configs = Parser::parse(configFile);
    if (configs.empty()) {
        std::cerr << "Error: No valid server configurations found in "
                  << configFile << std::endl;
        return -1;
    }

    for (size_t i = 0; i < configs.size(); i++) {
        int fd = create_socket(configs[i]);
        if (fd < 0) {
            close_all_sockets();
            return -1;
        }
        listen_fds.push_back(fd);
    }

    std::cout << "WebServ: " << listen_fds.size() << " sockets ready." << std::endl;
    return 0;
}

bool Server::is_listen_fd(int fd)
{
    for (size_t i = 0; i < listen_fds.size(); i++)
        if (listen_fds[i] == fd)
            return true;
    return false;
}

void Server::accept_client(std::vector<struct pollfd>& fds, int listen_fd)
{
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "Error: accept() failed: " << strerror(errno) << std::endl;
        return;
    }

    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error: fcntl() on client failed: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    struct pollfd pfd;
    pfd.fd      = client_fd;
    pfd.events  = POLLIN;
    pfd.revents = 0;
    fds.push_back(pfd);

    clients[client_fd] = Client();
    client_last_active[client_fd] = time(NULL);

    for (size_t i = 0; i < listen_fds.size(); i++) {
        if (listen_fds[i] == listen_fd) {
            client_config_index[client_fd] = i;
            break;
        }
    }

    std::cout << "New client connected (fd=" << client_fd << ")" << std::endl;
}

void Server::handle_cgi(std::vector<struct pollfd>& fds, size_t i)
{
    int         pipe_fd = fds[i].fd;
    CgiProcess& cgi     = cgi_processes[pipe_fd];

    char buffer[4096];
    int  bytes = read(pipe_fd, buffer, sizeof(buffer));

    if (bytes > 0)
    {
        cgi.output.append(buffer, bytes);
        return;
    }

    if (bytes < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << "Error: read() on CGI pipe failed: " << strerror(errno) << std::endl;
    }
    close(pipe_fd);
    fds.erase(fds.begin() + i);

    int status;
    waitpid(cgi.pid, &status, 0);

    Response res;
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        res = build_page_error(502, configs[cgi.config_index], cgi.location);
    else if (WIFSIGNALED(status))
        res = build_page_error(500, configs[cgi.config_index], cgi.location);
    else
        Response::parseCgiOutput(cgi.output, res);

    if (cgi.keep_alive)
        res.addHeader("connection", "keep-alive");
    else
        res.addHeader("connection", "close");

    std::string final = res.toString();
    send(cgi.client_fd, final.c_str(), final.size(), 0);

    if (cgi.keep_alive)
        clients[cgi.client_fd].reset();
    else
    {
        for (size_t j = 0; j < fds.size(); j++)
        {
            if (fds[j].fd == cgi.client_fd)
            {
                disconnect_client(fds, j);
                break;
            }
        }
    }

    cgi_processes.erase(pipe_fd);
}

void Server::handle_client(std::vector<struct pollfd>& fds, size_t i)
{
    int fd = fds[i].fd;
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        disconnect_client(fds, i);
        return;
    }
    client_last_active[fd] = time(NULL);
    const ServerConfig& config = configs[client_config_index[fd]];
    clients[fd].append_data(std::string(buffer, bytes), config);

    if (clients[fd].getErrorCode() != 0)
    {
        LocationConfig emptyLoc;
        Response response = build_page_error(clients[fd].getErrorCode(), configs[client_config_index[fd]], emptyLoc);
        std::string final = response.toString();
        send(fd, final.c_str(), final.size(), 0);
        disconnect_client(fds, i);
        return;
    }

    if (!clients[fd].is_complete())
        return;

    const HttpRequest& req = clients[fd].getRequest();

    const LocationConfig* location = NULL;
    size_t match = 0;

    for (size_t j = 0; j < config.locations.size(); j++) {
        const std::string& loc_path = config.locations[j].path;

        if (req.path == loc_path) {
            location = &config.locations[j];
            break;
        }
        if (req.path.find(loc_path) == 0) {
            bool valid = (loc_path == "/" || req.path[loc_path.size()] == '/');
            if (valid && loc_path.size() > match) {
                location = &config.locations[j];
                match    = loc_path.size();
            }
        }
    }
    if (!location) {
        LocationConfig emptyLoc;
        Response response = build_page_error(404, config, emptyLoc);
        std::string final = response.toString();
        send(fd, final.c_str(), final.size(), 0);
        disconnect_client(fds, i);
        return;
    }

    std::map<std::string, std::string>::const_iterator it =
        req.headers.find("connection");

    bool keep_alive = (it != req.headers.end() &&
                       it->second.find("keep-alive") != std::string::npos);

    std::string full_path = location->root + req.path;
    if (isCgi(full_path))
    {
        int pipe_in[2], pipe_out[2];
        if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
            send(fd, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n", 56, 0);
            disconnect_client(fds, i);
            return;
        }
        char **env = prepareEnv(req, full_path);
        pid_t pid  = fork();

        if (pid < 0)
        {
            std::cerr << "Error: fork() failed: " << strerror(errno) << std::endl;
            close(pipe_in[0]);  close(pipe_in[1]);
            close(pipe_out[0]); close(pipe_out[1]);
            freeEnv(env);
            send(fd, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n", 56, 0);
            disconnect_client(fds, i);
            return;
        }

        if (pid == 0)
        {
            dup2(pipe_in[0],  STDIN_FILENO);
            dup2(pipe_out[1], STDOUT_FILENO);
            close(pipe_in[0]);  close(pipe_in[1]);
            close(pipe_out[0]); close(pipe_out[1]);

            char *args[] = {
                (char *)location->cgi_path.c_str(),
                (char *)full_path.c_str(),
                NULL
            };
            execve(args[0], args, env);
            exit(1);
        }
        close(pipe_in[0]);
        if (req.method == "POST")
            write(pipe_in[1], req.body.c_str(), req.body.size());
        close(pipe_in[1]);
        close(pipe_out[1]);

        fcntl(pipe_out[0], F_SETFL, O_NONBLOCK);
        freeEnv(env);

        CgiProcess cgi;
        cgi.pipe_fd      = pipe_out[0];
        cgi.client_fd    = fd;
        cgi.pid          = pid;
        cgi.keep_alive   = keep_alive;
        cgi.config_index = client_config_index[fd];
        cgi.location     = *location;

        cgi_processes[pipe_out[0]] = cgi;

        struct pollfd pfd;
        pfd.fd      = pipe_out[0];
        pfd.events  = POLLIN;
        pfd.revents = 0;
        fds.push_back(pfd);

        std::cout << "CGI started (pipe_fd=" << pipe_out[0]
                  << " client_fd=" << fd << ")" << std::endl;
        return;
    }

    Response response = build_response(req, config, *location);

    if (keep_alive)
        response.addHeader("connection", "keep-alive");
    else
        response.addHeader("connection", "close");

    std::string final = response.toString();
    send(fd, final.c_str(), final.size(), 0);
    if (keep_alive)
        clients[fd].reset();
    else
        disconnect_client(fds, i);
}

void Server::run()
{
    std::vector<struct pollfd> fds;

    for (size_t i = 0; i < listen_fds.size(); i++) {
        struct pollfd pfd;
        pfd.fd      = listen_fds[i];
        pfd.events  = POLLIN;
        pfd.revents = 0;
        fds.push_back(pfd);
    }

    std::cout << "Server running ..." << std::endl;

    while (true)
    {
        int n = poll(fds.data(), fds.size(), 5000);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            std::cerr << "Error: poll() failed: " << strerror(errno) << std::endl;
            break;
        }

        check_timeouts(fds);

        for (size_t i = fds.size(); i-- > 0;)
        {
            if (!(fds[i].revents & POLLIN))
                continue;

            if (is_listen_fd(fds[i].fd))
                accept_client(fds, fds[i].fd);
            else if (cgi_processes.count(fds[i].fd))
                handle_cgi(fds, i);
            else
                handle_client(fds, i);
        }
    }
}