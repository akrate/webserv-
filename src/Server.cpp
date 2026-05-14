#include "../include/Server.hpp"


Server::Server() : epoll_fd(-1) {}

Server::~Server() {
    close_all_sockets();
    if (epoll_fd >= 0)
        close(epoll_fd);
}

void Server::close_all_sockets() {
    for (size_t i = 0; i < listen_fds.size(); i++)
        if (listen_fds[i] >= 0)
            close(listen_fds[i]);
    listen_fds.clear();
}

void Server::disconnect_client(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    clients.erase(fd);
    client_config_index.erase(fd);
    client_last_active.erase(fd);
    std::cout << "Client disconnected (fd=" << fd << ")" << std::endl;
}

void Server::check_timeouts()
{
    time_t now = time(NULL);
    std::map<int, time_t>::iterator it = client_last_active.begin();
    while (it != client_last_active.end())
    {
        int    fd      = it->first;
        time_t elapsed = now - it->second;
        ++it;
        if (elapsed >= CLIENT_TIMEOUT)
        {
            std::string msg =
                "HTTP/1.1 408 Request Timeout\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n";
            send(fd, msg.c_str(), msg.size(), 0);
            disconnect_client(fd);
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

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Error: epoll_create1() failed: " << strerror(errno) << std::endl;
        return -1;
    }

    for (size_t i = 0; i < configs.size(); i++) {
        int fd = create_socket(configs[i]);
        if (fd < 0) {
            close_all_sockets();
            return -1;
        }
        listen_fds.push_back(fd);

        struct epoll_event ev;
        ev.events  = EPOLLIN;
        ev.data.fd = fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            std::cerr << "Error: epoll_ctl() ADD listen_fd failed: "
                      << strerror(errno) << std::endl;
            close_all_sockets();
            return -1;
        }
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

void Server::accept_client(int listen_fd)
{
    while (true)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            std::cerr << "Error: accept() failed: " << strerror(errno) << std::endl;
            break;
        }

        if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
            std::cerr << "Error: fcntl() on client failed: " << strerror(errno) << std::endl;
            close(client_fd);
            continue;
        }

        struct epoll_event ev;
        ev.events  = EPOLLIN;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
            std::cerr << "Error: epoll_ctl() ADD client failed: " << strerror(errno) << std::endl;
            close(client_fd);
            continue;
        }

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
}

void Server::handle_client(int fd)
{
    client_last_active[fd] = time(NULL);

    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        disconnect_client(fd);
        return;
    }

    clients[fd].append_data(std::string(buffer, bytes),configs[client_config_index[fd]]);

    if (clients[fd].getErrorCode() != 0)
    {
        LocationConfig emptyLoc;
        Response response = build_page_error(clients[fd].getErrorCode(),configs[client_config_index[fd]],emptyLoc);
        std::string final = response.toString();
        send(fd, final.c_str(), final.size(), 0);
        disconnect_client(fd);
        return;
    }

    if (!clients[fd].is_complete())
        return;

    const HttpRequest&  req    = clients[fd].getRequest();
    const ServerConfig& config = configs[client_config_index[fd]];

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
        Response response = build_page_error(404,config,emptyLoc);
        std::string final = response.toString();
        send(fd, final.c_str(), final.size(), 0);
        disconnect_client(fd);
        return;
    }

    Response    response = build_response(req, config, *location);
    std::string final    = response.toString();
    send(fd, final.c_str(), final.size(), 0);

    std::cout << "Method : " << req.method     << std::endl;
    std::cout << "Path   : " << req.path       << std::endl;
    std::cout << "Host   : " << config.host    << std::endl;
    std::cout << "Port   : " << config.port    << std::endl;
    std::cout << "Root   : " << location->root << std::endl;

    disconnect_client(fd);
}

void Server::run()
{
    std::cout << "Server running ..." << std::endl;

    const int          MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

    while (true)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 5000);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            std::cerr << "Error: epoll_wait() failed: " << strerror(errno) << std::endl;
            break;
        }

        check_timeouts();

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;

            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                disconnect_client(fd);
                continue;
            }

            if (!(events[i].events & EPOLLIN))
                continue;

            if (is_listen_fd(fd))
                accept_client(fd);
            else
                handle_client(fd);
        }
    }
}