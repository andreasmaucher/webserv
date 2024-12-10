#include "../../include/serverConfig.hpp"

// // if no config file provided at execution call this constructor
// ServerConfig::ServerConfig() {
//     loadConfig(DEFAULT_CONFIG);    
// }

// else if config file is provided at execution call this constructor passing the argv[1] as the config file
// converting char* to string
// // ServerConfig::ServerConfig(const std::string &config_file) {
// //     loadConfig(config_file); // parser
// // }

const std::string &ServerConfig::getRootDirectory() const {
    return root_directory;
}

Route* ServerConfig::getRoute(const std::string &uri) {
    std::map<std::string, Route>::iterator it = routes.find(uri);
    if (it != routes.end()) {
        return &it->second;  // Return pointer to the found Route
    }
    return NULL;  // Return nullptr if not found
}

const std::map<std::string, Route> &ServerConfig::getRoutes() const {
    return routes;
}

const std::map<int, std::string> &ServerConfig::getErrorPages() const {
    return error_pages;
}

void ServerConfig::setRootDirectory(const std::string &root_directory) {
    this->root_directory = root_directory;
}

void ServerConfig::setRoute(const std::string &uri, const Route &route) {
    routes[uri] = route; // This will add a new route or update an existing one
}

// void ServerConfig::setRoutes(const std::map<std::string, Route> &routes) {
//     this->routes = routes;
// }

void ServerConfig::setErrorPage(const int &code, const std::string &path) {
    error_pages[code] = path;
}

void ServerConfig::setErrorPages(const std::map<int, std::string> &error_pages) {
    this->error_pages = error_pages;
}

// bool ServerConfig::loadConfig(const std::string &config_file) {
//     std::ifstream file(config_file);
//     if (!file.is_open()) {
//         std::cerr << "Error: could not open config file " << config_file << std::endl;
//         return false;
//     }

//     std::string line;
//     std::string uri;
//     std::string path;
//     std::string index_file;
//     std::set<std::string> methods;
//     std::set<std::string> content_type;
//     std::string redirect_uri;
//     bool directory_listing_enabled;
//     bool is_cgi;
//     while (std::getline(file, line)) {
//         if (line.empty() || line[0] == '#') {
//             continue;
//         }

//         std::istringstream iss(line);
//         std::string key;
//         iss >> key;
//         if (key == "root") {
//             iss >> root_directory;
//         } else if (key == "route") {
//             uri.clear();
//             path.clear();
//             methods.clear();
//             index_file.clear();
//             content_type.clear();
//             redirect_uri.clear();
//             directory_listing_enabled = false;
//             is_cgi = false;
//             std::string parameter;
//             while (iss >> parameter) {
//                 if (parameter == "uri") {
//                     iss >> uri;
//                 } else if (parameter == "path") {
//                     iss >> path;
//                 } else if (parameter == "method") {
//                     while (iss >> parameter) {
//                         methods.insert(parameter);
//                     }
//                 } else if (parameter == "content_type") {
//                     while (iss >> parameter) {
//                         content_type.insert(parameter);
//                     }
//                 } else if (parameter == "redirection") {
//                     iss >> redirect_uri;
//                 } else if (parameter == "index") {
//                         iss >> index_file;
//                 } else if (parameter == "directory_listing_enabled") {
//                     directory_listing_enabled = true;
//                 } else if (parameter == "is_cgi") {
//                     is_cgi = true;
//                 }
//             }
//             Route route;
//             route.uri = uri;
//             route.path = path;
//             route.methods = methods;
//             route.index = index_file;
//             route.content_type = content_type;
//             route.redirect_uri = redirect_uri;
//             route.directory_listing_enabled = directory_listing_enabled;
//             route.is_cgi = is_cgi;
//             routes[uri] = route;
//         }
//     }

//     file.close();
//     return true;
// }