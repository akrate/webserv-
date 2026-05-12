#ifndef SERVER_HPP
#define SERVER_HPP 
#include "webserv.hpp" 
#include "Parser.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <sys/epoll.h> 
#include "client.hpp"
#include <cstring> 
#include <ctime> 
#include "response.hpp"
class Client; 
class Server 
{ private: 
    std::vector<ServerConfig> 
    configs; std::vector<int> listen_fds; std::map<int, size_t> client_config_index; std::map<int, Client> clients; std::map<int, time_t> client_last_active; int epoll_fd; static const int CLIENT_TIMEOUT = 30; int create_socket(const ServerConfig& config); void close_all_sockets(); bool is_listen_fd(int fd); void accept_client(int listen_fd); void handle_client(int fd); void disconnect_client(int fd); void check_timeouts(); public: Server(); ~Server(); int init(const std::string& configFile); void run(); };
#endif