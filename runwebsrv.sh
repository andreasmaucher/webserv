#!/bin/bash

#if working from the host use:
docker run -it --rm -v "$(pwd)":/home/root webserv-img

## if working from the container use this option for exposing the port:
#docker run -it --rm -v "$(pwd)":/home/root -p 8080:8080 webserv-img