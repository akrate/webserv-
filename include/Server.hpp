#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <cstring>
#include <cerrno>
#include "Parser.hpp"   // for ParserException
#include "cgi_process.hpp"

class Client;

class Server {
    private:
        std::vector<ServerConfig>   configs;
        std::vector<int>            listen_fds;
        std::map<int, size_t>       client_config_index;
        std::map<int, Client>       clients;
        std::map<int, time_t>       client_last_active;
        std::map<int, CgiProcess>   cgi_processes;
        

        static const int            CLIENT_TIMEOUT = 30;

        int     create_socket(const ServerConfig& config);
        void    close_all_sockets();
        bool    is_listen_fd(int fd);
        void    disconnect_client(std::vector<struct pollfd>& fds, size_t i);
        void    accept_client(std::vector<struct pollfd>& fds, int listen_fd);
        void    handle_client(std::vector<struct pollfd>& fds, size_t i);
        void    handle_cgi(std::vector<struct pollfd>& fds, size_t i);
        void    check_timeouts(std::vector<struct pollfd>& fds);

    public:
        Server();
        ~Server();
        int     init(const std::string& configFile);
        void    run();
};

#endif