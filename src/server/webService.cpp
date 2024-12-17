/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webService.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cestevez <cestevez@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/12/17 17:27:55 by cestevez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/webService.hpp"

WebService::WebService(const std::string &config_file)
{
    signal(SIGINT, sigintHandler);
    parseConfig(config_file);
    setupSockets();
}

WebService::~WebService()
{
    // Server::cleanup();
    std::cout << "Service stopped" << std::endl;
}

// Return a listening socket
    // getaddrinfo(NULL, PORT, &hints, &ai)
    // NULL - hostname or IP address of the server, NULL means any available network, use for incoming connections
    // PORT - port
    // hints - criteria to resolve the ip address
    // ai - pointer to a linked list of results returned by getaddrinfo().
    // It holds the resolved network addresses that match the criteria specified in hints.
int WebService::get_listener_socket(const std::string port)
{
    (void)port;
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

void WebService::addToPfds(int new_fd, Server *server)
{
    struct pollfd new_pollfd;
    new_pollfd.fd = new_fd;
    new_pollfd.events = POLLIN | POLLOUT;
    new_pollfd.revents = 0;

    pfds_vec.push_back(new_pollfd);
    std::cout << "Added new fd: " << new_fd << " to pfds_vec at index: " << pfds_vec.size() - 1 << ". pfds_vec size: " << pfds_vec.size() << std::endl;

    pfd_to_server[new_fd] = server;
    std::cout << "Mapped fd: " << new_fd << " to server: " << server->name << std::endl;

}

void Server::del_from_pfds_vec(int fd)
{
    for (size_t i = 0; i < pfds_vec.size(); i++)
    {
        if (pfds_vec[i].fd == fd)
        {
            pfds_vec.erase(pfds_vec.begin() + i);
            std::cout << "Erased fd from vector at index: " << i << ". pfds_vec size: " << pfds_vec.size() << std::endl;
        }

    }
}

void Server::closeConnection(int &fd, int &i, std::vector<HttpRequest> &httpRequests) {
    std::cout << "Closing connection on fd: " << pfds_vec[i].fd << " at index: " << i << " in pfds vector due to error or client request." << std::endl;
    close(fd);
    std::cout << "Closed fd: " << pfds_vec[i].fd << " at index: " << i << " in pfds vector" << std::endl;
    del_from_pfds_vec(fd);
    httpRequests.erase(httpRequests.begin() + i);
    std::cout << "Erased request object at index:" << i << " from httpRequests vector" << std::endl;
}

void WebService::newConnection(Server *server)
{
    printf("\nNew connection!\n");
    addrlen = sizeof remoteaddr;
    int new_fd = accept(server.listener_fd, (struct sockaddr *)&remoteaddr, &addrlen);
    if (new_fd == -1)
    {
        perror("accept");
    }
    else
    {
        std::cout << "New connection accepted for server: " << server.name << " on fd: " << new_fd << std::endl;
        addToPfds(new_fd, &server);
        
        HttpRequest newRequest;
        httpRequests.push_back(newRequest);
        std::cout << "New httpRequest object created for connection on fd: " << new_fd << std::endl;
        pfd_to_request[new_fd] = &newRequest;
        std::cout << "Mapped fd: " << new_fd << " to its httpRequest object" << std::endl;
        }
}

//get a listening socket for each server
void WebService::setupSockets()
{
    for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it) {

        int listener_fd = get_listener_socket(it.port);
        if (it.listener_fd == -1)
        {
            //std::cout << "Error getting listening socket for server: " << it.name << std::endl;
            //exit(1);
            //maybe return to main and handle cleanup from there?
            throw std::runtime_error("Error getting listening socket for server: " + it.name);
        }
        // Store the listener_fd in the Server object
        it.listener_fd = listener_fd;

        // Add the listener_fd to pfds_vec and map it to this Server
        addToPfds(listener_fd, &(*it));
    }
    std::cout << "Total servers set up: " << servers.size() << std::endl;
    return ;
}

int Server::start()
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
            if (pfds_vec[i].revents & POLLIN)
            {
                Server *listening_server = NULL;
                for (std::vector <Server>::iterator it = servers.begin(); it != servers.end(); ++it)
                {
                    if (pfds_vec[i].fd == it.listener_fd) {
                        listening_server = &(*it);
                    }
                    // if (pfds_vec[i].fd == it.listener_fd)
                    // {
                    //     newConnection(&(*it)); //passing the server object that is receiving the new connection
                    // } else {
                    //     receiveRequest(&(*it));
                    // }
                }
                if (listening_server != NULL)
                {
                    newConnection(listening_server); //passing the server object that is receiving the new connection
                } else {
                    receiveRequest(&(*it));
                }
            }
            else if (pfds_vec[i].revents & POLLOUT)
            {
                // check if httpRequests is complete!
                sendResponse(pfds_vec[i].fd);
            }
        }
    }
}

// implement timeout for recv()?
void Server::receiveRequest(int i)
{
    std::cout << "Receive function called for request at index: " << i << " . Size of httpRequests vector: " << httpRequests.size() << std::endl;
    if (!httpRequests[i].complete)
    {        
        int nbytes = recv(pfds_vec[i].fd, buf, sizeof buf, 0);
        if (nbytes <= 0)
        {
            closeConnection(pfds_vec[i].fd, i, httpRequests);
        }
        else
        {
            buf[nbytes] = '\0';
            httpRequests[i].raw_request.append(buf);
            std::cout << "Received data from fd: " << pfds_vec[i].fd << " at index: " << i << " from pfds vector" << std::endl;
            if (httpRequests[i].raw_request.find(END_HEADER) != std::string::npos)
            {
                std::cout << "Parsing request from request object at index:" << i << std::endl;
                RequestParser::parseRawRequest(httpRequests[i]);
            }
        }
    }
}

void Server::sendResponse(int i, HttpRequest &request)
{
    std::cout << "Send function called for request at index:" << i << std::endl;
    std::cout << "Request complete: " << request.complete << std::endl;

    HttpResponse response;
    ResponseHandler::processRequest(config, request, response);

    std::string responseStr = response.generateRawResponseStr();
    if (send(pfds_vec[i].fd, responseStr.c_str(), responseStr.size(), 0) == -1)
    {
        perror("send");
    }
    std::cout << "Response sent to fd: " << pfds_vec[i].fd << " at index: " << i << " in pfds_vec" << std::endl;
    if (response.close_connection == true)
    {
        closeConnection(pfds_vec[i].fd, i, httpRequests);
    } else {
        request.reset();
    }
}

void Server::sigintHandler(int signal)
{
    if (signal == SIGINT)
    {
        printf("Ctrl- C Closing connection\n");
        printf("Exiting\n");
        // Memory leaks will be present, since we can't run freeaddrinfo(res) unless it is declared as a global?
        exit(0);
    }
}
