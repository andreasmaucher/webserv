#include "../../include/server.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/webService.hpp"

// Server::Server() {}

Server::Server(int listener_fd, std::string port, std::string name, std::string root_directory) : listener_fd(listener_fd), port(port), name(name), root_directory(root_directory) {}

Server::Server() : listener_fd(-1), port(""), name(""), host(""), root_directory("")
{
    // Initialize other members if needed
}

const std::string &Server::getPort() const
{
    return port;
}

const std::string &Server::getName() const
{
    return name;
}

const int &Server::getListenerFd() const
{
    return listener_fd;
}

const std::string &Server::getRootDirectory() const
{
    return root_directory;
}

Route *Server::getRoute(const std::string &uri)
{
    std::map<std::string, Route>::iterator it = routes.find(uri);
    if (it != routes.end())
    {
        return &it->second; // Return pointer to the found Route
    }
    return NULL; // Return nullptr if not found
}

const std::map<std::string, Route> &Server::getRoutes() const
{
    return routes;
}

const std::map<int, std::string> &Server::getErrorPages() const
{
    return error_pages;
}

HttpRequest &Server::getRequestObject(int &fd)
{
    return this->client_fd_to_request[fd];
}

void Server::setRootDirectory(const std::string &root_directory)
{
    this->root_directory = root_directory;
}

void Server::setRoute(const std::string &uri, const Route &route)
{
    routes[uri] = route; // This will add a new route or update an existing one
}

// void Server::setRoutes(const std::map<std::string, Route> &routes) {
//     this->routes = routes;
// }

void Server::setErrorPage(const int &code, const std::string &path)
{
    error_pages[code] = path;
}

void Server::setErrorPages(const std::map<int, std::string> &error_pages)
{
    this->error_pages = error_pages;
}

void Server::setListenerFd(const int &listener_fd)
{
    this->listener_fd = listener_fd;
}

void Server::setRequestObject(int &fd, HttpRequest &request)
{
    this->client_fd_to_request[fd] = request;
}

void Server::deleteRequestObject(const int &fd)
{
    int fd_to_delete = fd;
    this->client_fd_to_request.erase(fd_to_delete);
}

void Server::resetRequestObject(int &fd)
{
    this->client_fd_to_request[fd].reset();
}

void Server::debugServer() const
{
    DEBUG_MSG("Server Name", name);
    DEBUG_MSG("Port", port);
    DEBUG_MSG("Listener FD", listener_fd);
    DEBUG_MSG("Host", host);
    DEBUG_MSG("Root Directory", root_directory);
    DEBUG_MSG("Client Max Body Size", client_max_body_size);
    DEBUG_MSG("Index", index);

    if (error_pages.empty())
    {
        DEBUG_MSG("Error Pages", "None configured");
        return;
    }

    for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it)
    {
        DEBUG_MSG("Error Code ", it->first);
        DEBUG_MSG("Error Code ", it->second);
    }
    DEBUG_MSG("Server DEBUG", "----------------------------------------");
}

void Server::debugPrintRoutes() const
{
    DEBUG_MSG("Routes DEBUG for Server", name);
    DEBUG_MSG("(Port: ", port);

    if (routes.empty())
    {
        DEBUG_MSG("Routes Status", "No routes configured");
        return;
    }

    std::map<std::string, Route>::const_iterator it;
    for (it = routes.begin(); it != routes.end(); ++it)
    {
        // const std::string &uri = it->first;
        const Route &route = it->second;

        // DEBUG_MSG("Route URI", uri);
        DEBUG_MSG("Route Path", route.path);

        std::string methods_str;
        for (std::set<std::string>::const_iterator method_it = route.methods.begin();
             method_it != route.methods.end(); ++method_it)
        {
            methods_str += *method_it + " ";
        }
        DEBUG_MSG("Allowed Methods", methods_str);

        std::string types_str;
        for (std::set<std::string>::const_iterator type_it = route.content_type.begin();
             type_it != route.content_type.end(); ++type_it)
        {
            types_str += *type_it + " ";
        }
        DEBUG_MSG("Content Types", types_str);

        DEBUG_MSG("Redirect URI", route.redirect_uri);
        DEBUG_MSG("Index File", route.index_file);
        DEBUG_MSG("Directory Listing", route.directory_listing_enabled);
        DEBUG_MSG("CGI Status", route.is_cgi);
        DEBUG_MSG("Route DEBUG", "----------------------------------------");
    }
}

void Server::clear()
{
    host.clear();
    port.clear();
    name.clear();
    root_directory.clear();
    client_max_body_size.clear();
    index.clear();
    routes.clear();
    error_pages.clear();
}

// bool Server::loadConfig(const std::string &config_file) {
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