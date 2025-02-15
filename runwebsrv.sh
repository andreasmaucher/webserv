!/bin/bash

set -x  # Enable DEBUG mode to see what's happening

#if testing from inside of the container use:
#docker run -it --rm -v "$(pwd)":/home/root webserv-img

## if working from outside of the container use this option for exposing the port:
docker run -it --rm -v "$(pwd)":/home/root -p 8080:8080 webserv-img sh -c "make re && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./webserv tomldb.config"
# docker run -it --rm -v "$(pwd)":/home/root -p 8080:8080 webserv-img sh -c "make re && ./webserv tomldb.config"
