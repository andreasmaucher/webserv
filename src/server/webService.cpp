/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webService.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cestevez <cestevez@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/12/20 14:59:40 by cestevez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/webService.hpp"

WebService::WebService(const std::string &config_file)
{
    (void)config_file;
    signal(SIGINT, sigintHandler);
    //servers = ConfigParser::parseConfig(config_file);
    servers = createFakeServers();
    setupSockets();
}

WebService::~WebService()
{
    // WebService::cleanup();
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

    std::cout << "Connecting on port " << port << std::endl;
    if ((addrinfo_status = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0)
    {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(addrinfo_status));
        exit(1);
    }
    int listener_fd = -1;
    for (p = ai; p != NULL; p = p->ai_next)
    { // loop through the ai ll and try to create a socket
        listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener_fd < 0)
        {
            continue;
        }
        // Lose the pesky "address already in use" error message
        // Manually added option to allow to reuse ports straight after closing the server - SO_REUSEADDR
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_socket_opt, sizeof(reuse_socket_opt)) < 0)
        {
            perror("setsockopt(SO_REUSEADDR) failed");
            exit(1);
        }
        // tries to bind the socket, otherwise closes it
        if (bind(listener_fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener_fd);
            continue;
        }
        // breaks loop on success
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

void WebService::closeConnection(int &fd, size_t &i, Server &server) {
    std::cout << "Closing connection on fd: " << fd << std::endl;
    
    close(fd);
    std::cout << "Closed fd: " << fd << std::endl;
    
    deleteFromPfdsVec(fd, i);
    deleteRequestObject(fd, server);


    std::cout << "Erased request object for fd: " << fd << std::endl;
}

//creates a new httpRequest object for the new connection in the server object and maps the fd to the new httpRequest object
void WebService::createRequestObject(int new_fd, Server &server) {
    HttpRequest newRequest;
    
    //server.client_fd_to_request[new_fd] = &newRequest;
    server.setRequestObject(new_fd, newRequest);
    std::cout << "New httpRequest object created for connection on fd: " << new_fd << std::endl;
    std::cout << "Mapped fd: " << new_fd << " to its httpRequest object" << std::endl;
}

void WebService::mapFdToServer(int new_fd, Server &server) {
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

//get a listening socket for each server
void WebService::setupSockets()
{
    for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it) {

        int listener_fd = get_listener_socket((*it).getPort());
        if (listener_fd == -1)
        {
            //std::cout << "Error getting listening socket for server: " << it.name << std::endl;
            //exit(1);
            //maybe return to main and handle cleanup from there?
            throw std::runtime_error("Error getting listening socket for server: " + (*it).getName());
        }
        // Store the listener_fd in the Server object
        (*it).setListenerFd(listener_fd);

        // Add the listener_fd to pfds_vec and map it to this Server
        addToPfdsVector(listener_fd);
        mapFdToServer(listener_fd, *it);
        //fd_to_server[listener_fd] = &(*it);
    }
    std::cout << "Total servers set up: " << servers.size() << std::endl;
    return ;
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
                if (pollfd_obj.fd == server_obj->getListenerFd())
                {
                    newConnection(*server_obj);
                } else {
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
    //HttpRequest *request_obj = server.client_fd_to_request[fd];
    HttpRequest request_obj = server.getRequestObject(fd);
    //std::cout << "Receive function called for request at index: " << i << " . Size of httpRequests vector: " << httpRequests.size() << std::endl;
    if (!request_obj.complete)
    {        
        int nbytes = recv(fd, buf, sizeof buf, 0);
        if (nbytes <= 0)
        {
            closeConnection(fd, i, server);
        }
        else
        {
            buf[nbytes] = '\0';
            request_obj.raw_request.append(buf);
            std::cout << "Received data from fd: " << fd << std::endl;
            if (request_obj.raw_request.find(END_HEADER) != std::string::npos)
            {
                //std::cout << "Parsing request from request object at index:" << i << std::endl;
                RequestParser::parseRawRequest(request_obj);
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
        } else {
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
        // Memory leaks will be present, since we can't run freeaddrinfo(res) unless it is declared as a global?
        exit(0);
    }
}
