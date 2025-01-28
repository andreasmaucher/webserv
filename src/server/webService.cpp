/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webService.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/01/28 19:16:00 by mrizhakov        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/webService.hpp"
#include "../../include/Parser.hpp"

std::vector<pollfd> WebService::pfds_vec;
std::vector<Server> WebService::servers;
std::map<int, Server *> WebService::fd_to_server;

WebService::WebService(const std::string &config_file)
{
    signal(SIGINT, sigintHandler);
    Parser parser;
    servers = parser.parseConfig(config_file);
    DEBUG_MSG("Configured servers", servers.size());

    for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
    {
        (*it).debugServer();
        (*it).debugPrintRoutes();
    }
    setupSockets();
}

void WebService::cleanup()
{
    for (size_t i = 0; i < pfds_vec.size(); i++)
    {
        close(pfds_vec[i].fd);
    }
    pfds_vec.clear();

    // Close all server listener sockets
    for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
    {
        if ((*it).getListenerFd() != -1)
        {
            close((*it).getListenerFd());
        }
    }

    fd_to_server.clear(); // Clear the map
}

WebService::~WebService()
{
    WebService::cleanup();
    DEBUG_MSG("Service status", "stopped");
}

// Return a listening socket
// getaddrinfo(NULL, PORT, &hints, &ai)
// NULL - hostname or IP address of the server, NULL means any available network, use for incoming connections
// PORT - port
// hints - criteria to resolve the ip address
// ai - pointer to a linked list of results returned by getaddrinfo().
// It holds the resolved network addresses that match the criteria specified in hints.
int WebService::get_listener_socket(const std::string &port)
{
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    reuse_socket_opt = 1;

    DEBUG_MSG("Attempting connection on port", port);

    if ((addrinfo_status = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0)
    {
        DEBUG_MSG_1("getaddrinfo error", gai_strerror(addrinfo_status));
        return -1;
    }

    int listener_fd = -1;
    for (p = ai; p != NULL; p = p->ai_next)
    {
        DEBUG_MSG("Socket creation", "in progress");
        listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener_fd < 0)
        {
            DEBUG_MSG_1("socket() error", strerror(errno));
            continue;
        }

        DEBUG_MSG("Setting SO_REUSEADDR", "in progress");
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_socket_opt, sizeof(reuse_socket_opt)) < 0)
        {
            DEBUG_MSG_1("setsockopt error", strerror(errno));
            close(listener_fd);
            continue;
        }

        DEBUG_MSG("Setting SO_REUSEPORT", "in progress");
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_socket_opt, sizeof(reuse_socket_opt)) < 0)
        {
            DEBUG_MSG_1("setsockopt error", strerror(errno));
            close(listener_fd);
            continue;
        }

        DEBUG_MSG("Binding socket", "in progress");
        if (bind(listener_fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            DEBUG_MSG_1("bind() error", strerror(errno));
            close(listener_fd);
            continue;
        }
        break;
    }

    if (p == NULL)
        return -1;

    freeaddrinfo(ai);
    ai = NULL;
    errno = 0;
    if (listen(listener_fd, MAX_BACKLOG_UNACCEPTED_CON) == -1)
    {
        DEBUG_MSG_1("Listend failed, error", strerror(errno));
        return -1;
    }

    return listener_fd;
}

void WebService::addToPfdsVector(int new_fd)
{
    struct pollfd new_pollfd;
    new_pollfd.fd = new_fd;
    new_pollfd.events = POLLIN | POLLOUT;
    new_pollfd.revents = 0;

    pfds_vec.push_back(new_pollfd);
    DEBUG_MSG("Added new fd to pfds_vec", new_fd);
    DEBUG_MSG("Current pfds_vec size", pfds_vec.size());
}

void WebService::deleteFromPfdsVec(int &fd, size_t &i)
{
    if (pfds_vec[i].fd == fd)
    {
        pfds_vec.erase(pfds_vec.begin() + i);
        DEBUG_MSG("Erased fd from vector at index", i);
        DEBUG_MSG("Current pfds_vec size", pfds_vec.size());
    }
    else
    {
        DEBUG_MSG_1("Error: fd not found in vector at index", i);
    }
}

void WebService::deleteRequestObject(int &fd, Server &server)
{
    server.deleteRequestObject(fd);
}

void WebService::closeConnection(int &fd, size_t &i, Server &server)
{
    if (close(fd) == -1)
        DEBUG_MSG_1("Closing connection FD failed", strerror(errno));

    deleteFromPfdsVec(fd, i);
    deleteRequestObject(fd, server);
    DEBUG_MSG("Erased request object for fd", fd);
}

// creates a new httpRequest object for the new connection in the server object and maps the fd to the new httpRequest object
void WebService::createRequestObject(int new_fd, Server &server)
{
    HttpRequest newRequest;
    server.setRequestObject(new_fd, newRequest);
    DEBUG_MSG("New httpRequest object created for fd", new_fd);
}

void WebService::mapFdToServer(int new_fd, Server &server)
{
    this->fd_to_server[new_fd] = &server;
}

void WebService::newConnection(Server &server)
{
    DEBUG_MSG("New connection", "incoming");
    struct sockaddr_storage remoteaddr;
    addrlen = sizeof remoteaddr;
    int new_fd = accept(server.getListenerFd(), (struct sockaddr *)&remoteaddr, &addrlen);

    errno = 0;
    if (new_fd == -1)
    {
        DEBUG_MSG_1("Accept error", strerror(errno));
    }
    else
    {
        DEBUG_MSG("New connection accepted for server ", server.getName());
        DEBUG_MSG("On fd", new_fd);

        addToPfdsVector(new_fd);
        mapFdToServer(new_fd, server);
        createRequestObject(new_fd, server);
    }
}

// get a listening socket for each server
void WebService::setupSockets()
{
    for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
    {
        errno = 0;
        int listener_fd = get_listener_socket((*it).getPort());
        if (listener_fd == -1)
        {
            DEBUG_MSG_1("get_listener_socket failed", strerror(errno));

            throw std::runtime_error("Error getting listening socket for server: " + (*it).getName());
        }
        (*it).setListenerFd(listener_fd);
        addToPfdsVector(listener_fd);
        mapFdToServer(listener_fd, *it);
    }
    DEBUG_MSG("Total servers set up", servers.size());
}

int WebService::start()
{
    DEBUG_MSG("Server Status", "Starting");
    while (true)
    {
        // DEBUG_MSG("Connection Status", "Waiting for connections");
        errno = 0;
        int poll_count = poll(pfds_vec.data(), pfds_vec.size(), POLL_TIMEOUT);

        if (poll_count == -1)
        {
            DEBUG_MSG_1("Poll error", strerror(errno));
            continue;
        }

        for (size_t i = 0; i < pfds_vec.size(); i++)
        {
            pollfd pollfd_obj = pfds_vec[i];
            Server *server_obj = fd_to_server[pollfd_obj.fd];

            if (pollfd_obj.revents & POLLIN)
            {
                if (i < servers.size())
                    newConnection(*server_obj);
                else
                    receiveRequest(pollfd_obj.fd, i, *server_obj);
            }
            else if (pollfd_obj.revents & POLLOUT)
            {
                sendResponse(pollfd_obj.fd, i, *server_obj);
            }
            // else if (pollfd_obj.revents & POLLPRI)
            // {
            //     DEBUG_MSG_1("------------>POLL says POLLPRI", "");
            // }
            // else if (pollfd_obj.revents & POLLERR)
            // {
            //     DEBUG_MSG_1("------------>POLL says POLLERR", "");
            // }
            // else if (pollfd_obj.revents & POLLHUP)
            // {
            //     DEBUG_MSG_1("------------>POLL says POLLHUP", "");
            // }
            // else if (pollfd_obj.revents & POLLNVAL)
            // {
            //     DEBUG_MSG_1("------------>POLL says POLLNVAL", "");
            // }
            // else
            // {
            //     DEBUG_MSG_1("------------>POLL says SOME OTHER ERROR", strerror(errno));
            // }
        }
    }
}

// implement timeout for recv()?
void WebService::receiveRequest(int &fd, size_t &i, Server &server)
{
    DEBUG_MSG("=== RECEIVE REQUEST ===", "");
    DEBUG_MSG("Processing FD", fd);

    if (!server.getRequestObject(fd).complete)
    {
        errno = 0;
        int nbytes = recv(fd, buf, sizeof buf, 0);
        DEBUG_MSG("Bytes received", nbytes);

        if (nbytes == 0)
        {
            DEBUG_MSG("Receive closed succesfully", nbytes);
            closeConnection(fd, i, server);
        }
        else if (nbytes < 0)
        {
            DEBUG_MSG_1("Receive failed, error", strerror(errno));
            closeConnection(fd, i, server);
        }
        else
        {
            buf[nbytes] = '\0';
            server.getRequestObject(fd).raw_request.append(buf);
            DEBUG_MSG("Received data from fd", fd);
            DEBUG_MSG("Data received", buf);
            DEBUG_MSG("Request object raw_request", server.getRequestObject(fd).raw_request);

            // Try to parse headers if we haven't yet
    if (!server.getRequestObject(fd).headers_parsed && 
        server.getRequestObject(fd).raw_request.find("\r\n\r\n") != std::string::npos)
    {
        DEBUG_MSG("Request Status", "Parsing headers");
        try {
            RequestParser::parseRawRequest(server.getRequestObject(fd));
        } catch (const std::exception& e) {
            DEBUG_MSG("Error parsing request", e.what());
            closeConnection(fd, i, server);
            return;
        }
    }

    // If headers are parsed, try to parse body
    if (server.getRequestObject(fd).headers_parsed)
    {
        try {
            RequestParser::parseRawRequest(server.getRequestObject(fd));
        } catch (const std::exception& e) {
            DEBUG_MSG("Error parsing body", e.what());
            closeConnection(fd, i, server);
            return;
        }
    }

    DEBUG_MSG("Current body size", server.getRequestObject(fd).body.size());
    if (server.getRequestObject(fd).complete)
    {
        DEBUG_MSG("Request complete", "Ready to process");
    }
    }
}

void WebService::sendResponse(int &fd, size_t &i, Server &server)
{
    HttpRequest request_obj = server.getRequestObject(fd);
    if (request_obj.complete)
    {
        HttpRequest request = server.getRequestObject(fd);
        HttpResponse response;
        ResponseHandler handler;
        handler.processRequest(fd, server, request, response);

        std::string responseStr = response.generateRawResponseStr();
        if (send(fd, responseStr.c_str(), responseStr.size(), 0) == -1)
        {
            DEBUG_MSG_1("Send error ", strerror(errno));
        }
        DEBUG_MSG("Response sent to fd", fd);

        if (response.close_connection == true)
        {
            closeConnection(fd, i, server);
        }
        else
        {
            server.resetRequestObject(fd);
        }
    }
}

void WebService::sigintHandler(int signal)
{
    if (signal == SIGINT)
    {
        DEBUG_MSG("Signal received", "SIGINT");
        DEBUG_MSG("Server status", "Shutting down");
        WebService::cleanup();
        exit(0);
    }
}

// std::vector<Server> WebService::parseConfig(const std::string &config_file)
// {
//     std::vector<Server> servers_vector;
//     if (!parseConfigFile(config_file))
//     {
//         std::cerr << "Error: Failed to parse config file" << std::endl;
//         return servers_vector;
//     }

//     // Example of config of fake servers

//     // Server server_one(0, "8080", "server_one", "www");

//     // // Add a route for static files
//     // Route staticRoute;
//     // staticRoute.uri = "/static";
//     // staticRoute.path = server_one.getRootDirectory() + staticRoute.uri;
//     // staticRoute.index_file = "index.html";
//     // staticRoute.methods.insert("GET");
//     // staticRoute.methods.insert("POST");
//     // staticRoute.methods.insert("DELETE");
//     // staticRoute.content_type.insert("text/plain");
//     // staticRoute.content_type.insert("text/html");
//     // staticRoute.is_cgi = false;
//     // server_one.setRoute(staticRoute.uri, staticRoute);

//     // // Add a route for more restrictive folder in static
//     // Route restrictedstaticRoute;
//     // restrictedstaticRoute.uri = "/static/restrictedstatic";
//     // restrictedstaticRoute.path = server_one.getRootDirectory() + restrictedstaticRoute.uri;
//     // restrictedstaticRoute.index_file = "index.html";
//     // restrictedstaticRoute.methods.insert("GET");
//     // restrictedstaticRoute.content_type.insert("text/plain");
//     // restrictedstaticRoute.content_type.insert("text/html");
//     // restrictedstaticRoute.is_cgi = false;
//     // server_one.setRoute(restrictedstaticRoute.uri, restrictedstaticRoute);

//     // test_servers.push_back(server_one);

//     // Server server_two(0, "8081", "server_two", "www");

//     // // Add a route for images
//     // Route imageRoute;
//     // imageRoute.uri = "/images";
//     // imageRoute.path = server_one.getRootDirectory() + imageRoute.uri;
//     // imageRoute.methods.insert("GET");
//     // imageRoute.methods.insert("POST");
//     // // imageRoute.methods.insert("DELETE");
//     // imageRoute.content_type.insert("image/jpeg");
//     // imageRoute.content_type.insert("image/png");
//     // imageRoute.is_cgi = false;
//     // server_two.setRoute(imageRoute.uri, imageRoute);

//     // // Add a route for uploads
//     // Route uploadsRoute;
//     // uploadsRoute.uri = "/uploads";
//     // uploadsRoute.path = server_one.getRootDirectory() + uploadsRoute.uri;
//     // uploadsRoute.methods.insert("GET");
//     // uploadsRoute.methods.insert("POST");
//     // uploadsRoute.methods.insert("DELETE");
//     // uploadsRoute.content_type.insert("image/jpeg");
//     // uploadsRoute.content_type.insert("image/png");
//     // uploadsRoute.content_type.insert("text/plain");
//     // uploadsRoute.content_type.insert("text/html");
//     // uploadsRoute.is_cgi = false;
//     // server_two.setRoute(uploadsRoute.uri, uploadsRoute);

//     // test_servers.push_back(server_two);

//     return servers_vector;
// }

// bool WebService::parseConfigFile(const std::string &config_filename)
// {
//     std::ifstream config_file(config_filename.c_str());
//     if (!config_file.is_open())
//     {
//         std::cerr << "Error: can't open config file" << std::endl;
//         return false;
//     }

//     bool found_server = false;
//     std::string line;

//     while (std::getline(config_file, line))
//     {
//         if (line.empty() || line[0] == '#')
//             continue;

//         if (line.find("[[server]]") != std::string::npos)
//         {
//             if (found_server)
//             {
//                 // Create a new config and copy current values
//                 Server new_config(port, host, name, root_directory);

//                 new_config.host = host;
//                 new_config.port = port;
//                 new_config.root_directory = root_directory;
//                 new_config.index = index;
//                 new_config.client_max_body_size = client_max_body_size;
//                 new_config.routes = routes;
//                 new_config.error_pages = error_pages;
//                 configs.push_back(new_config);

//                 // Reset current values for next server
//                 host.clear();
//                 port = 0;
//                 root_directory.clear();
//                 index.clear();
//                 client_max_body_size.clear();
//                 routes.clear();
//                 error_pages.clear();
//             }

//             if (!parseServerBlock(config_file))
//                 return false;

//             found_server = true;
//         }
//     }

//     if (found_server)
//     {
//         // Add the last server config
//         ServerConfig new_config;
//         new_config.host = host;
//         new_config.port = port;
//         new_config.root_directory = root_directory;
//         new_config.index = index;
//         new_config.client_max_body_size = client_max_body_size;
//         new_config.routes = routes;
//         new_config.error_pages = error_pages;
//         configs.push_back(new_config);
//     }

//     DEBUG_MSG("Parsed " << configs.size() << " server configurations" << std::endl;

//     // Use first config as main config
//     if (!configs.empty())
//     {
//         host = configs[0].host;
//         port = configs[0].port;
//         root_directory = configs[0].root_directory;
//         index = configs[0].index;
//         client_max_body_size = configs[0].client_max_body_size;
//         routes = configs[0].routes;
//         error_pages = configs[0].error_pages;

//         configs.erase(configs.begin());
//     }

//     DEBUG_MSG("First server port: " << port << std::endl;
//     return found_server;
// }