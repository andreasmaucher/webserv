/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/01/05 23:06:29 by mrizhakov        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/server.hpp"

Server::Server(const std::string &config_file)
{
    // Create and parse config
    config = ServerConfig(config_file);

    // Use the first server's port by default
    char port_buf[32];
    snprintf(port_buf, sizeof(port_buf), "%d", config.getPort());
    std::string port_str = port_buf;

    std::cout << "Using first server's port: " << port_str << std::endl;

    if (!setup(port_str))
    {
        throw std::runtime_error("Failed to setup server");
    }
}

Server::~Server()
{
    // Added cleanup to exit conditions, need to check if it is always triggered
    // TODO: Maybe worth puttting back in the destructor
    // Server::cleanup();
    std::cout << "Server exited" << std::endl;
}

void *Server::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
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
    int listener; // Listening socket descriptor
    int yes = 1;  // For setsockopt() SO_REUSEADDR

    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv;
    if ((rv = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0)
    {
        std::cerr << "getaddrinfo error: " << gai_strerror(rv) << std::endl;
        return -1;
    }

    // Loop through all the results and bind to the first we can
    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            std::cerr << "socket() error: " << strerror(errno) << std::endl;
            continue;
        }

        // Lose the "address already in use" error message
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        {
            std::cerr << "setsockopt() error: " << strerror(errno) << std::endl;
            close(listener);
            continue;
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            std::cerr << "bind() error: " << strerror(errno) << std::endl;
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai);

    if (p == NULL)
    {
        std::cerr << "Failed to bind to any address" << std::endl;
        return -1;
    }

    if (listen(listener, MAX_SIM_CONN) == -1)
    {
        std::cerr << "listen() error: " << strerror(errno) << std::endl;
        return -1;
    }

    return listener;
}

void Server::add_to_pfds_vec(int newfd)
{
    struct pollfd pfd;
    pfd.fd = newfd;
    pfd.events = POLLIN; // Check ready-to-read
    pfd.revents = 0;
    pfds_vec.push_back(pfd);

    std::cout << "Added new fd: " << newfd << " to pfds_vec at index: "
              << pfds_vec.size() - 1 << ". pfds_vec size: " << pfds_vec.size() << std::endl;
}

// Removing and index from the set
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

void Server::closeConnection(int &fd, int &i, std::vector<HttpRequest> &httpRequests)
{
    std::cout << "Closing connection on fd: " << pfds_vec[i].fd << " at index: " << i << " in pfds vector due to error or client request." << std::endl;
    close(fd);
    std::cout << "Closed fd: " << pfds_vec[i].fd << " at index: " << i << " in pfds vector" << std::endl;
    del_from_pfds_vec(fd);
    httpRequests.erase(httpRequests.begin() + i); // Erase the HttpRequest for this fd
    std::cout << "Erased request object at index:" << i << " from httpRequests vector" << std::endl;
}

void Server::new_connection()
{

    printf("\nNew connection!\n");
    addrlen = sizeof remoteaddr;
    new_fd = accept(listener_fd, (struct sockaddr *)&remoteaddr, &addrlen);
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
    std::cout << "Connecting on port " << port << std::endl;

    // Get listener socket
    listener_fd = get_listener_socket(port);
    if (listener_fd == -1)
    {
        std::cerr << "Error getting listening socket" << std::endl;
        return 0;
    }

    // Add the listener to pfds_vec
    add_to_pfds_vec(listener_fd);

    std::cout << "Server: waiting for connections on port " << port << std::endl;
    return 1; // Return success
}

int Server::start()
{
    while (1)
    {
        poll_count = poll(pfds_vec.data(), pfds_vec.size(), -1);
        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }
        // Run through the existing connections looking for data to read
        for (size_t i = 0; i < pfds_vec.size(); i++)
        {
            // do the POLLIN and POLLOUT checks actually work or just using request.complete?
            if (pfds_vec[i].revents & POLLIN)
            {
                if (pfds_vec[i].fd == listener_fd) // initial created fd at index 0 is the listener
                {
                    new_connection();
                }
                else
                { // we are in the fd of an existing connection
                    receiveRequest(i);
                }
            }
            // POLLOUT is set when the socket is ready to send data (bc not receiving anymore)
            if (pfds_vec[i].revents & POLLOUT)
            {
                sendResponse(i, httpRequests[i]);
            }
        }
    }
}

void Server::receiveRequest(int i)
{
    std::cout << "Request function called for request at index: " << i << " . Size of httpRequests vector: " << httpRequests.size() << std::endl;
    if (!httpRequests[i].complete) // i < static_cast<int>(httpRequests.size()) &&
    {
        int nbytes = recv(pfds_vec[i].fd, buf, sizeof buf, 0); // Receive data from client
        if (nbytes <= 0)
        {
            // if (nbytes == 0)
            // {
            //     printf("Connection interrupted or closed by client. ");
            // }
            // else
            // {
            //     perror("recv");
            // }
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
    std::cout << "Response function called for request at index:" << i << std::endl;
    std::cout << "Request complete: " << request.complete << std::endl;
    if (request.complete)
    {
        HttpResponse response;
        ResponseHandler::processRequest(config, request, response);

        std::string responseStr = response.generateRawResponseStr();
        std::cout << "Response string: " << responseStr << std::endl;
        if (send(pfds_vec[i].fd, responseStr.c_str(), responseStr.size(), 0) == -1)
        {
            perror("send");
        }
        std::cout << "Response sent to fd: " << pfds_vec[i].fd << " at index: " << i << " in pfds_vec" << std::endl;
        if (response.close_connection == true)
        {
            closeConnection(pfds_vec[i].fd, i, httpRequests);
        }
        else
        {
            // Keep connection open for further requests from same client, cleaning the request object
            pfds_vec[i].revents = POLLIN; // Set to listen for more requests
            request.reset();
        }
    }
}

void Server::cleanup()
{
    if (new_fd != -1)
    {
        close(new_fd);
    }
    if (ai != NULL)
    {
        freeaddrinfo(ai);
    }
    // TODO: change malloc to new and free
    // if (pfds_vec)
    //     delete[] pfds;
}

void Server::sigintHandler(int signal)
{
    if (signal == SIGINT)
    {
        printf("Ctrl- C Closing connection\n"); // Print the message.
        // TODO: should figure out a way to cleanup memory and close fd's
        //  cleanup();
        //  close(new_fd);

        printf("Exiting\n");
        // Memory leaks will be present, since we can't run freeaddrinfo(res) unless it is declared as a global
        exit(0);
    }
}
