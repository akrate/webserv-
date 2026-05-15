#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include <string>
#include <sys/types.h>

struct CgiProcess {
    int            pipe_fd;
    int            client_fd;
    pid_t          pid;
    bool           keep_alive;
    std::string    output;
    size_t         config_index;  // ← add
    LocationConfig location;      // ← add
};

#endif