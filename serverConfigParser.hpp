#ifndef SERVERCONFIGPARSER.HPP
#define SERVERCONFIGPARSER.HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class ServerConfig {
    std::string host;
    int port;
    std::vector<std::string> server_names;
    std::map<int, std::string> error_pages;  // Error code -> page file
    size_t client_body_size_limit;
    struct Location {
        std::string path;
        std::string root;
        std::string index;
        std::vector<std::string> accepted_methods;
        bool directory_listing;
        bool redirect;
        std::string redirect_url;
        std::string cgi_extension;
    };
    std::vector<Location> locations;
};

#endif