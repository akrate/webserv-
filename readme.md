# webserv

> 42 School — C++98 HTTP/1.1 server inspired by Nginx

---

## Table of Contents

- [About](#about)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [HTTP Methods](#http-methods)
- [CGI Support](#cgi-support)
- [Project Structure](#project-structure)
- [Resources](#resources)

---

## About

**webserv** is a project from the [42 School](https://42.fr) core curriculum. The goal is to build a fully functional HTTP/1.1 web server from scratch in **C++98**, without using any existing web server implementations. The server behaves similarly to Nginx — it reads a configuration file and handles client connections in a non-blocking, multiplexed manner using `poll()` (or equivalent).

---

## Features

- HTTP/1.1 compliant (RFC 7230–7235)
- Non-blocking I/O with `poll()` for all socket operations
- Nginx-style configuration file parsing
- Multiple virtual servers on different ports/hosts
- HTTP methods: `GET`, `POST`, `DELETE`
- Static file serving with auto-index (directory listing)
- Custom error pages
- File upload handling
- CGI execution (Python, PHP, etc.)
- Configurable request body size limit
- HTTP redirections

---

## Requirements

- C++98 compatible compiler (`g++` or `clang++`)
- `make`
- UNIX-like OS (Linux / macOS)
- Python 3 or PHP (optional, for CGI)

---

## Installation

```bash
# Clone the repository
git clone https://github.com/your-username/webserv.git
cd webserv

# Compile
make
```

To clean build artifacts:

```bash
make clean    # Remove object files
make fclean   # Remove object files and binary
make re       # Full rebuild
```

---

## Configuration

The server takes a `.conf` file as its argument. If none is provided, it falls back to `config/default.conf`.

```bash
./webserv config/default.conf
```

### Example configuration

```nginx
server {
    listen       8080;
    host         127.0.0.1;
    server_name  localhost;

    root         ./public/www;
    index        index.html;

    client_max_body_size 10M;

    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;

    location / {
        methods     GET POST;
        autoindex   off;
    }

    location /upload {
        methods     POST DELETE;
        upload_dir  ./public/uploads;
    }

    location /cgi-bin {
        methods     GET POST;
        cgi_ext     .py  /usr/bin/python3;
        cgi_ext     .php /usr/bin/php;
    }

    location /old-path {
        return 301 /new-path;
    }
}
```

### Configuration directives

| Directive               | Scope           | Description                                   |
|-------------------------|-----------------|-----------------------------------------------|
| `listen`                | server          | Port to listen on                             |
| `host`                  | server          | IP address to bind                            |
| `server_name`           | server          | Virtual host name                             |
| `root`                  | server/location | Root directory for file serving               |
| `index`                 | server/location | Default file to serve for directories         |
| `client_max_body_size`  | server          | Max allowed request body size                 |
| `error_page`            | server          | Custom error page per HTTP status code        |
| `methods`               | location        | Allowed HTTP methods                          |
| `autoindex`             | location        | Enable directory listing (`on`/`off`)         |
| `upload_dir`            | location        | Directory where uploaded files are stored     |
| `cgi_ext`               | location        | CGI extension and interpreter path            |
| `return`                | location        | HTTP redirection (code + target URL)          |

---

## Usage

```bash
# Start with default config
./webserv

# Start with a custom config
./webserv config/mysite.conf
```

The server logs requests to stdout. Stop it with `Ctrl+C`.

---

## HTTP Methods

| Method   | Description                        |
|----------|------------------------------------|
| `GET`    | Retrieve a resource or static file |
| `POST`   | Submit data or upload files        |
| `DELETE` | Delete a specified resource        |

---

## CGI Support

Place CGI scripts in a directory covered by a `location` block with `cgi_ext` defined. The server forks a child process, passes the request environment via standard CGI variables, and returns the script's stdout as the HTTP response.

**Supported CGI variables include:**

- `REQUEST_METHOD`
- `QUERY_STRING`
- `CONTENT_TYPE`
- `CONTENT_LENGTH`
- `SCRIPT_FILENAME`
- `PATH_INFO`
- `SERVER_NAME` / `SERVER_PORT`

Example CGI script (`hello.py`):

```python
#!/usr/bin/env python3
print("Content-Type: text/html\r\n\r\n")
print("<h1>Hello from CGI!</h1>")
```

---

## Project Structure

```
webserv/
├── config/
│   └── default.conf        # Default configuration file
├── public/
│   ├── www/                # Static files root
│   │   └── index.html
│   ├── uploads/            # Upload destination
│   └── errors/             # Custom error pages
├── cgi-bin/                # CGI scripts
├── srcs/
│   ├── main.cpp
│   ├── Server.cpp/.hpp     # Socket, poll loop
│   ├── Config.cpp/.hpp     # Configuration parser
│   ├── Request.cpp/.hpp    # HTTP request parser
│   ├── Response.cpp/.hpp   # HTTP response builder
│   └── CGI.cpp/.hpp        # CGI handler
├── includes/
├── Makefile
└── README.md
```

---

## Resources

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/)
- [HTTP/1.1 RFC 7230](https://datatracker.ietf.org/doc/html/rfc7230)
- [Building a simple HTTP server from scratch](https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa)
- [Nginx configuration docs](https://nginx.org/en/docs/)

---

*Made with ☕ at 42 School.*
