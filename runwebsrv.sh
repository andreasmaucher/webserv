#!/bin/bash

set -x  # Enable DEBUG mode to see what's happening

#if testing from inside of the container use:
#docker run -it --rm -v "$(pwd)":/home/root webserv-img

## Use this option for exposing the port:
docker run -it --rm --name webserv -p 8080:8080 -v "$(pwd)":/home/root webserv-img sh -c "cd /home/root && make re && valgrind ./webserv tomldb.config"
# The --network host approach is being replaced with proper port mapping (-p 8080:8080)
# pfds_vec.reserve(pfds_vec.capacity() * 2 + 1);