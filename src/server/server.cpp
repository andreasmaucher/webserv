/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cestevez <cestevez@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/12/13 15:10:29 by cestevez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/server.hpp"

Server::Server(const std::string &port) : pfds_vec(1)
{
    signal(SIGINT, sigintHandler);
    setup(port);
}

Server::~Server()
{
    // Server::cleanup();
    std::cout << "Server exited" << std::endl;
}

// Return a listening socket
    // getaddrinfo(NULL, PORT, &hints, &ai)
    // NULL - hostname or IP address of the server, NULL means any available network, use for incoming connections
    // PORT - port
    // hints - criteria to resolve the ip address
    // ai - pointer to a linked list of results returned by getaddrinfo().
    // It holds the resolved network addresses that match the criteria specified in hints.
int Server::get_listener_socket(const std::string port)
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

void Server::add_to_pfds_vec(int newfd)
{
    struct pollfd new_pollfd;
    new_pollfd.fd = newfd;
    new_pollfd.events = POLLIN | POLLOUT;
    new_pollfd.revents = 0;

    pfds_vec.push_back(new_pollfd);
    std::cout << "Added new fd: " << newfd << " to pfds_vec at index: " << pfds_vec.size() - 1 << ". pfds_vec size: " << pfds_vec.size() << std::endl;
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

void Server::new_connection()
{

    printf("\nNew connection!\n");
    addrlen = sizeof remoteaddr;
    int new_fd = accept(listener_fd, (struct sockaddr *)&remoteaddr, &addrlen);
    if (new_fd == -1)
    {
        perror("accept");
    }
    else
    {
        std::cout << "New connection accepted on fd: " << new_fd << std::endl;
        add_to_pfds_vec(new_fd);
        HttpRequest newRequest;
        httpRequests.push_back(newRequest);
        std::cout << "New httpRequest instance created for connection on fd: " << new_fd << " at index: " << httpRequests.size() - 1 << " . HttpRequests vector size: " << httpRequests.size() << std::endl;

    }
}


int Server::setup(const std::string &port)
{
    (void)port;

    // hardcoding server config for testing
    config = createFakeServerConfig();

    listener_fd = get_listener_socket(port);
    if (listener_fd == -1)
    {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    pfds_vec[0].fd = listener_fd;
    pfds_vec[0].events = POLLIN | POLLOUT;
    pfds_vec[0].revents = 0;

    HttpRequest newRequest;
    httpRequests.push_back(newRequest);
    std::cout << "pfds_vec size at setup: " << pfds_vec.size() << std::endl;
    std::cout << "httpRequest vector size at setup: " << httpRequests.size() << std::endl;
    
    return (listener_fd);
}

int Server::start()
{
    while (1)
    {
        // poll changes the state of the pfds_vec; POLLOUT is the default state of the sockets (writable) unless theres incoming data detected
        poll_count = poll(pfds_vec.data(), pfds_vec.size(), -1);
        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }
        // Run through all existing fds, sending or receiving data depending on POLL status; or create a new connection if fd 0 (listener)
        for (size_t i = 0; i < pfds_vec.size(); i++)
        {
            if (pfds_vec[i].revents & POLLIN)
            {
                if (pfds_vec[i].fd == listener_fd)
                {
                    new_connection();
                } else {
                    receiveRequest(i);
                }
            }
            if (pfds_vec[i].revents & POLLOUT && httpRequests[i].complete)
            {
                sendResponse(i, httpRequests[i]);
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
