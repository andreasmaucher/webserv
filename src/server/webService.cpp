/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webService.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/01/11 18:26:26 by mrizhakov        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/webService.hpp"
#include "../../include/Parser.hpp"

std::vector<pollfd> WebService::pfds_vec;
std::vector<Server> WebService::servers;

WebService::WebService(const std::string &config_file)
{
    signal(SIGINT, sigintHandler);
    // servers = ConfigParser::parseConfig(config_file);
    // servers = createFakeServers();
    // servers = parseConfig(config_file);
    Parser parser;

    servers = parser.parseConfig(config_file);

    std::cout << "Configured " << servers.size() << " servers." << std::endl;
    servers[0].debugPrintRoutes();
    servers[1].debugPrintRoutes();
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
}

WebService::~WebService()
{
    WebService::cleanup();
    std::cout << "Service stopped" << std::endl;
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
    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // Unspecified IPv4 or IPv6, can use both
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // Fill in the IP address of the server automatically when getaddrinfo(), for listening sockets
    reuse_socket_opt = 1;

    std::cout << "Trying to connect on port: " << port << std::endl;

    if ((addrinfo_status = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0)
    {
        std::cerr << "getaddrinfo error: " << gai_strerror(addrinfo_status) << std::endl;
        return -1;
    }

    int listener_fd = -1;
    for (p = ai; p != NULL; p = p->ai_next)
    {
        std::cout << "Creating socket..." << std::endl;
        listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener_fd < 0)
        {
            std::cerr << "socket() error: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "Setting socket option SO_REUSEADDR..." << std::endl;
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_socket_opt, sizeof(reuse_socket_opt)) < 0)
        {
            std::cerr << "setsockopt error: " << strerror(errno) << std::endl;
            close(listener_fd);
            continue;
        }

        std::cout << "Setting socket option SO_REUSEPORT..." << std::endl;
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_socket_opt, sizeof(reuse_socket_opt)) < 0)
        {
            std::cerr << "setsockopt error: " << strerror(errno) << std::endl;
            close(listener_fd);
            continue;
        }

        std::cout << "Binding socket..." << std::endl;
        if (bind(listener_fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            std::cerr << "bind() error: " << strerror(errno) << std::endl;
            close(listener_fd);
            continue;
        }
        break;
    }
    // If we got here, it means we didn't get bound. Reached end of ai ll
    if (p == NULL)
    {
        return -1;
    }
    freeaddrinfo(ai);
    ai = NULL;

    // Starts listening for incoming connections on the listener socket, with a maximum backlog of 10 connections waiting to be accepted.
    if (listen(listener_fd, MAX_SIM_CONN) == -1)
    {
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
    std::cout << "Added new fd: " << new_fd << " to pfds_vec at index: " << pfds_vec.size() - 1 << ". pfds_vec size: " << pfds_vec.size() << std::endl;
}

void WebService::deleteFromPfdsVec(int &fd, size_t &i)
{
    if (pfds_vec[i].fd == fd)
    {
        pfds_vec.erase(pfds_vec.begin() + i);
        std::cout << "Erased fd from vector at index: " << i << ". pfds_vec size: " << pfds_vec.size() << std::endl;
    }
    else
    {
        std::cout << "Error: fd not found in vector at index: " << i << " Check the logic!" << std::endl;
    }
}

void WebService::deleteRequestObject(int &fd, Server &server)
{
    server.deleteRequestObject(fd);
}

void WebService::closeConnection(int &fd, size_t &i, Server &server)
{
    std::cout << "Closing connection on fd: " << fd << std::endl;

    close(fd);
    std::cout << "Closed fd: " << fd << std::endl;

    deleteFromPfdsVec(fd, i);
    deleteRequestObject(fd, server);

    std::cout << "Erased request object for fd: " << fd << std::endl;
}

// creates a new httpRequest object for the new connection in the server object and maps the fd to the new httpRequest object
void WebService::createRequestObject(int new_fd, Server &server)
{
    HttpRequest newRequest;

    // server.client_fd_to_request[new_fd] = &newRequest;
    server.setRequestObject(new_fd, newRequest);
    std::cout << "New httpRequest object created for connection on fd: " << new_fd << std::endl;
    std::cout << "Mapped fd: " << new_fd << " to its httpRequest object" << std::endl;
}

void WebService::mapFdToServer(int new_fd, Server &server)
{
    this->fd_to_server[new_fd] = &server;
}

void WebService::newConnection(Server &server)
{
    printf("\nNew connection!\n");
    addrlen = sizeof remoteaddr;
    int new_fd = accept(server.getListenerFd(), (struct sockaddr *)&remoteaddr, &addrlen);
    if (new_fd == -1)
    {
        perror("accept");
    }
    else
    {
        std::cout << "New connection accepted for server: " << server.getName() << " on fd: " << new_fd << std::endl;
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

        int listener_fd = get_listener_socket((*it).getPort());
        if (listener_fd == -1)
        {
            // std::cout << "Error getting listening socket for server: " << it.name << std::endl;
            // exit(1);
            // maybe return to main and handle cleanup from there?
            throw std::runtime_error("Error getting listening socket for server: " + (*it).getName());
        }
        // Store the listener_fd in the Server object
        (*it).setListenerFd(listener_fd);

        // Add the listener_fd to pfds_vec and map it to this Server
        addToPfdsVector(listener_fd);
        mapFdToServer(listener_fd, *it);
        // fd_to_server[listener_fd] = &(*it);
    }
    std::cout << "Total servers set up: " << servers.size() << std::endl;
    return;
}

int WebService::start()
{
    while (true)
    {
        // poll changes the state of the pfds_vec; POLLOUT is the default state of the sockets (writable) unless theres incoming data detected
        int poll_count = poll(pfds_vec.data(), pfds_vec.size(), -1);
        if (poll_count == -1)
        {
            throw std::runtime_error("poll failed");
            // perror("poll");
            // exit(1);
        }
        // Run through all existing fds, sending or receiving data depending on POLL status; or create a new connection if fd 0 (listener)
        for (size_t i = 0; i < pfds_vec.size(); i++)
        {
            pollfd pollfd_obj = pfds_vec[i];
            Server *server_obj = fd_to_server[pollfd_obj.fd];
            if (pollfd_obj.revents & POLLIN)
            {
                // if (pollfd_obj.fd == server_obj->getListenerFd()) simplified:
                if (i < servers.size()) // it's a listener
                {
                    newConnection(*server_obj);
                }
                else
                {
                    receiveRequest(pollfd_obj.fd, i, *server_obj);
                }
            }
            else if (pollfd_obj.revents & POLLOUT)
            {
                // check if httpRequest is complete!
                sendResponse(pollfd_obj.fd, i, *server_obj);
            }
        }
    }
}

// implement timeout for recv()?
void WebService::receiveRequest(int &fd, size_t &i, Server &server)
{
    // HttpRequest *request_obj = server.client_fd_to_request[fd];
    //  HttpRequest request_obj = server.getRequestObject(fd);
    std::cout << "Receive function called for request on server: " << server.getName() << " - listener fd: " << server.getListenerFd() << " fd from pollfds vector: " << fd << std::endl;

    if (!server.getRequestObject(fd).complete)
    {
        int nbytes = recv(fd, buf, sizeof buf, 0);
        if (nbytes <= 0)
        {
            closeConnection(fd, i, server);
        }
        else
        {
            buf[nbytes] = '\0';
            server.getRequestObject(fd).raw_request.append(buf);
            std::cout << "Received data from fd: " << fd << std::endl;
            std::cout << "Data received: " << buf << std::endl;
            std::cout << "Request object raw_request: " << server.getRequestObject(fd).raw_request << std::endl;
            if (server.getRequestObject(fd).raw_request.find(END_HEADER) != std::string::npos)
            {
                std::cout << "Parsing request..." << std::endl;
                RequestParser::parseRawRequest(server.getRequestObject(fd));
            }
        }
    }
}

void WebService::sendResponse(int &fd, size_t &i, Server &server)
{
    // std::cout << "Send function called for request at index:" << i << std::endl;
    // std::cout << "Request complete?: " << request.complete << std::endl;
    HttpRequest request_obj = server.getRequestObject(fd);
    if (request_obj.complete)
    {
        HttpRequest request = server.getRequestObject(fd);
        HttpResponse response;
        ResponseHandler::processRequest(server, request, response);

        std::string responseStr = response.generateRawResponseStr();
        if (send(fd, responseStr.c_str(), responseStr.size(), 0) == -1)
        {
            perror("send");
        }
        std::cout << "Response sent to fd: " << fd << " at index: " << i << " in pfds_vec" << std::endl;
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
        printf("Ctrl- C Closing connection\n");
        printf("Exiting\n");
        WebService::cleanup();
        // Memory leaks will be present, since we can't run freeaddrinfo(res) unless it is declared as a global?
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

//     std::cout << "Parsed " << configs.size() << " server configurations" << std::endl;

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

//     std::cout << "First server port: " << port << std::endl;
//     return found_server;
// }