#ifndef SERVER_HPP
#define SERVER_HPP

#include "webserv.hpp"
#include "Parser.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>

class Server {
    private:
        std::vector<ServerConfig>   configs;
        std::vector<int>            listen_fds;

        int create_socket(const ServerConfig& config);
        void close_all_sockets();

        bool is_listen_fd(int fd);
        void accept_client(std::vector<struct pollfd>& fds, int listen_fd);
        void handle_client(std::vector<struct pollfd>& fds, size_t i);
    public:
        Server();
        ~Server();
        int init(const std::string& configFile);
        void run();
};
#endif