#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include <string>
#include <sys/types.h>

struct CgiProcess {
    int         pipe_fd;
    int         client_fd;
    pid_t       pid;
    std::string output;
    bool        keep_alive;
};

#endif