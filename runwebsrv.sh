#!/bin/bash
docker run -it --rm -v "$(pwd)":/home/root -p 8080:8080 webserv-img