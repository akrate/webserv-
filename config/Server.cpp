#include "Server.hpp"


Server::Server() {
}

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
    fds.erase(fds.begin() + i);
}
int Server::create_socket(const ServerConfig& config)
{
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if (fd < 0)
    {
        std::cerr << "Error :socket() failed for port"
             << config.port << ": " << strerror(errno) <<
             std::endl;
        return -1;
    }
    int opt = 1;
    if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))< 0)
    {
        std::cerr << "Error : setsockopt() failed for port"
         << config.port << ": " << strerror(errno) << std::endl;
         close (fd);
         return -1;
    }
    if (fcntl(fd,F_SETFL,O_NONBLOCK) < 0)
    {
        std::cerr << "Error : fcntl () failed for port"
         << config.port << ": "<< strerror(errno) << std::endl;
         close (fd);
         return -1;
    }
    struct sockaddr_in addr;
    memset (&addr,0,sizeof(addr));
    addr.sin_family =AF_INET;
    addr.sin_port = htons(config.port);
    if (inet_pton(AF_INET,config.host.c_str(),&addr.sin_addr)<=0)
    {
        std::cerr << "Error: invalid host address ' " << config.host << "'"
        << std::endl;
        close (fd);
        return (-1);
    }
    if (bind(fd,(struct sockaddr*)&addr,sizeof(addr))< 0)

    {
        std::cerr << "Error bind failed on" << config.host 
        << ":" << config.port << " - " <<strerror(errno) << std::endl;
        close (fd);
        return -1;
    }
    if (listen (fd, 128) < 0 )
    {
        std::cerr << "Error : listen failed on port"
        << config.host << ": " << strerror(errno) << std::endl;
        close (fd);
        return (-1);
    }
    std::cout << "Listening on "<< config.host << ":"<< config.port << " (fd =" << fd << ")" <<
    std::endl;
    return fd;
} 
int Server::init(const std::string& configFile) {
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
 
    std::cout << "WebServ: " << listen_fds.size()
    << " sockets ready." << std::endl;
    return 0;
}

//-------------------------poll---

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
    for (size_t i = 0; i < listen_fds.size(); i++) {
        if (listen_fds[i] == listen_fd) {
            client_config_index[client_fd] = i;
            break;
        }
    }
    std::cout << "New client connected (fd=" << client_fd << ")" << std::endl;
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
    clients[fd].append_data(std::string(buffer, bytes));
    if (clients[fd].getErrorCode() != 0) {
        disconnect_client(fds, i);
        return;
    }
    if (!clients[fd].is_complete())
        return;

    const HttpRequest&   req    = clients[fd].getRequest();
    const ServerConfig&  config = configs[client_config_index[fd]];

    const LocationConfig* location = NULL;
    for (size_t j = 0; j < config.locations.size(); j++) {
        if (req.path.find(config.locations[j].path) == 0) {
            location = &config.locations[j];
            break;
        }
    }
    if (!location) {
        std::string err = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(fd, err.c_str(), err.size(), 0);
        disconnect_client(fds, i);
        return;
    }
    std::cout << "Method : " << req.method  << std::endl;
    std::cout << "Path   : " << req.path    << std::endl;
    std::cout << "Host   : " << config.host << std::endl;
    std::cout << "Port   : " << config.port << std::endl;
    std::cout << "Root   : " << location->root << std::endl;
    std::cout << req.method << " " << req.path << " -> " << location->root << std::endl;
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
        int n = poll(fds.data(), fds.size(), -1);
        if (n < 0) {
            std::cerr << "Error: poll() failed: " << strerror(errno) << std::endl;
            break;
        }
 
        for (size_t i = fds.size(); i-- > 0;)
        {
            if (!(fds[i].revents & POLLIN))
                continue;
 
            if (is_listen_fd(fds[i].fd))
                accept_client(fds, fds[i].fd);
            else
                handle_client(fds, i);
        }
    }
}
 