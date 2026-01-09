# webserv

A custom HTTP/1.1 web server implementation in C++98, built from scratch as part of the 42 curriculum. This project implements a fully functional web server capable of handling multiple concurrent connections, serving static files, executing CGI scripts, and processing various HTTP methods.

## Overview

This server handles GET, POST, and DELETE requests, supports multiple virtual hosts, executes Python CGI scripts, and includes features like directory listing, file uploads, custom error pages, and HTTP redirects. The implementation uses non-blocking I/O with `poll()` for efficient connection management.

## Core Technologies

- **C++98** - Standard compliance (no modern C++ features)
- **Socket Programming** - Raw socket API (socket, bind, listen, accept)
- **I/O Multiplexing** - `poll()` for handling multiple connections
- **Process Management** - Fork/exec for CGI execution with pipes
- **TOML Parsing** - Custom configuration file parser
- **HTTP/1.1 Protocol** - Full request/response cycle implementation

## Features

- Multiple server blocks with different ports and configurations
- HTTP methods: GET, POST, DELETE
- CGI execution for Python scripts (.py, .cgi)
- File upload handling
- Directory auto-indexing
- Custom error pages for various HTTP status codes
- HTTP redirects
- MIME type detection
- Request body size limits
- Non-blocking I/O with event-driven architecture

## Build & Run

```bash
make
./webserv [config_file]
```

If no config file is provided, defaults to `tomldb.config`.

## Configuration

The server uses a TOML-based config format. Example:

```toml
[[server]]
listen = 8080
host = "127.0.0.1"
root = "www"
index = "index.html"
client_max_body_size = 100000
allow_methods = ["GET", "POST", "DELETE"]
cgi_ext = [".py", ".cgi"]
cgi_path = "/cgi-bin"

[[server.location]]
uri = "/"
autoindex = true
allow_methods = ["GET", "POST", "DELETE"]
upload_dir = "www/uploads/"
```

## Technical Highlights

**Event-Driven Architecture**: Implemented non-blocking I/O using `poll()` for efficient connection handling. Designed a state machine to manage request/response cycles, handling partial reads, large payloads, and connection state transitions between POLLIN and POLLOUT events.

**CGI Execution**: Built process management system for CGI scripts using fork/exec with bidirectional pipe communication. Implemented timeout handling, zombie process cleanup, and coordinated data flow between CGI processes and client sockets.

**HTTP/1.1 Protocol Implementation**: Full request parsing including chunked transfer encoding, multipart form data, and proper header validation. Handles edge cases such as malformed requests, oversized payloads, and various content encodings per RFC 7230-7237.

**Resource Management**: Manual memory and file descriptor management in C++98, ensuring proper cleanup on errors and client disconnects. Implemented connection state tracking across the event loop with robust error recovery.

## Project Structure

```
webserv/
├── include/          # Headers
├── src/
│   ├── server/      # Server setup, config parsing
│   ├── http/        # Request/response handling
│   └── cgi/         # CGI execution
├── cgi-bin/         # Python CGI scripts
├── www/             # Web root
└── tomldb.config    # Server configuration
```

## Testing

The server can be tested with standard tools:

```bash
# Basic GET
curl http://localhost:8080/

# File upload
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads/

# DELETE
curl -X DELETE http://localhost:8080/uploads/test.txt

# CGI
curl http://localhost:8080/cgi-bin/list_files.py
```

## Docker

A Docker setup is included for consistent development:

```bash
docker build -t webserv-img .
./runwebsrv.sh
```

---
