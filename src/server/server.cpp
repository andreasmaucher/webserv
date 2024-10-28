/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizakov <mrizakov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/27 20:20:43 by mrizakov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "cgi.hpp"

void *Server::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    { // Check if using IPv4
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    // Else return IPv6 address
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// Return a listening socket
int Server::get_listener_socket(const std::string port)
{
    (void)port;
    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // Unspecified IPv4 or IPv6, can use both
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // Fill in the IP address of the server automatically when getaddrinfo(), for listening sockets
    reuse_socket_opt = 1;

    // getaddrinfo(NULL, PORT, &hints, &ai)
    // NULL - hostname or IP address of the server, NULL means any available network, use for incoming connections
    // PORT - port
    // hints - criteria to resolve the ip address
    // ai - pointer to a linked list of results returned by getaddrinfo().
    // It holds the resolved network addresses that match the criteria specified in hints.

    std::cout << "Going to connect on port " << port << std::endl;
    if ((addrinfo_status = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0)
    {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(addrinfo_status));
        exit(1);
    }
    // gai_strerror(addrinfo_status) - use this instead of perror() or strerror() since it is designed to describe getaddrinfo() or other network function error codes

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
    // Cleaning up ai ll
    freeaddrinfo(ai); // All done with this
    ai = NULL;

    // Listening
    // Starts listening for incoming connections on the listener socket, with a maximum backlog of 10 connections waiting to be accepted.
    if (listen(listener_fd, MAX_SIM_CONN) == -1)
    {
        return -1;
    }
    // returns fd of the listening socket
    return listener_fd;
}

// Add a new fd to the set
void Server::add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If the pollfd struct doesnt have space, add more space
    if (*fd_count == *fd_size)
    {                  // if struct full, add space
        *fd_size *= 2; // Double the size

        // TODO: switch  realloc to resize and use vectors instead of array for pfds
        *pfds = (struct pollfd *)realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }
    (*pfds)[*fd_count].fd = newfd;                // add new fd at position fd_count of pfds struct
    (*pfds)[*fd_count].events = POLLIN | POLLOUT; // Set the fd for reading

    (*fd_count)++; // add fd_count to keep track of used fd's in pfds struct
}

// Removing and index from the set
void Server::del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // i - index of fd to be removed
    // fd_count - pointer to number of fd's being monitored
    pfds[i] = pfds[*fd_count - 1]; // replace the fd to be removed (fd[i]) with the last element of array
    (*fd_count)--;                 // make the size of the array smaller by one
    // Not doing realloc() to change the actual size of the array, since this can be slow
}

Server::Server(const std::string &port) : sockfd(-1), new_fd(-1)
{
    signal(SIGINT, sigintHandler);
    setup(port);
}

Server::~Server()
{
    // Added cleanup to exit conditions, need to check if it is always triggered
    // TODO: Maybe worth puttting back in the destructor
    // Server::cleanup();
    std::cout << "Server exited" << std::endl;
}

int Server::setup(const std::string &port)
{
    (void)port;
    fd_count = 0;           // Current number of used fds
    fd_size = INIT_FD_SIZE; // Initial size of struct to hold all fds
    HttpRequest newRequest;
    for (int i = 0; i < fd_size; i++)
        httpRequests.push_back(newRequest);
    request_fully_received = 0;
    response_ready_to_send = 0;

    // TODO: change to vector
    pfds = new struct pollfd[fd_size]();

    listener_fd = get_listener_socket(port);
    if (listener_fd == -1)
    {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    pfds[0].fd = listener_fd;
    pfds[0].events = POLLIN | POLLOUT;
    ;             // Watch for incoming data on the listener
    fd_count = 1; // There is only 1 listener on the socket
    return (listener_fd);
    // Step 1 END: SETUP
}

int Server::start()
{
    // STEP 2 START: MAIN LOOP
    // Main loop
    // bool request_complete;
    while (1)
    {
        poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }

        // Run through the existing connections looking for data to read
        for (int i = 0; i < fd_count; i++)
        {
            if ((pfds[i].revents & POLLIN) && pfds[i].fd == listener_fd)
            {
                new_connection(i);
            }
            // Check if an fd is ready to read
            if (pfds[i].revents & POLLIN)
            {
                request(i);
            }
            // Received a connection
            if (pfds[i].revents & POLLOUT)
            {
                response(i);
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
    } // END  while(1) loop
    // Step 2 END: MAIN LOOP
}

void Server::new_connection(int i)
{
    (void)i;
    printf("Accept happens here \n");
    addrlen = sizeof remoteaddr;
    new_fd = accept(listener_fd, (struct sockaddr *)&remoteaddr, &addrlen);
    if (new_fd == -1)
    {
        perror("accept");
    }
    else
    {
        add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);
        HttpRequest newRequest;             // Assuming HttpRequest has a default constructor
        httpRequests.push_back(newRequest); // Add it to the vector
        printf("pollserver: new connection from %s on socket %d\n",
               inet_ntop(remoteaddr.ss_family,
                         get_in_addr((struct sockaddr *)&remoteaddr),
                         remoteIP, INET6_ADDRSTRLEN),
               new_fd);
    }
}

void Server::request(int i)
{
    if (!httpRequests[i].request_completed)
    {
        // If not the listener, we're just the regular client
        // printf("At recv fn \n");
        int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
        int sender_fd = pfds[i].fd;
        if (nbytes <= 0)
        {
            // Got error or connection closed by client
            if (nbytes == 0)
            {
                // Connection closed
                printf("pollserver: socket %d hung up \n", sender_fd);
            }
            else
            {
                perror("recv");
            }
            close(pfds[i].fd); // Closing fd
            del_from_pfds(pfds, i, &fd_count);
            httpRequests.erase(httpRequests.begin() + i); // Erase the HttpRequest for this fd
        }
        else
        {
            // Received some good data from the client
            // Printing out received data
            buf[nbytes] = '\0';
            httpRequests[i].raw_request.append(buf);
            if (httpRequests[i].raw_request.find(END_HEADER) != std::string::npos)
            {
                std::cout << "Full request from client " << i << " is: " << httpRequests[i].raw_request << std::endl;
                std::cout << "Found end of request command \"close\", stopped reading request " << std::endl;
                // PARSER COMES HERE
                //! ANDY
                // Check if the request is a CGI request
                if (CGI::isCGIRequest(httpRequests[i].uri))
                {
                    // Create a CGI object and handle the request
                    CGI cgi(sender_fd, httpRequests[i].uri, httpRequests[i].method, httpRequests[i].queryString, httpRequests[i].body);
                    cgi.handleCGIRequest(httpRequests[i]);
                }
                //  httpRequests[i].request_completed = RequestParser::parseRawRequest(httpRequests[i]);
                httpRequests[i].request_completed = 1;
                // break;
            }
        } // END handle data from client
    } // END got ready-to-read from poll()
}

void Server::response(int i)
{
    if (httpRequests[i].request_completed)
    {
        std::cout << "Server response (echo original request): " << httpRequests[i].raw_request << std::endl;
        // std::string response = "Server response (echo original request):  " + httpRequests[i].raw_request;
        std::string response =
            "HTTP/1.1 200 OK\r\nDate: Fri, 27 Oct 2023 14:30:00 GMT\r\nServer: CustomServer/1.0\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nConnection: keep-alive\r\n\r\nHello, World!\r\n";
        // TODO: RESPONSE GOES HERE
        if (send(pfds[i].fd, response.c_str(), response.size(), 0) == -1)
        {
            perror("send");
        }
        httpRequests[i].request_completed = 0;
        httpRequests[i].raw_request = "";
    }
}

void Server::cleanup()
{
    if (new_fd != -1)
    {
        close(new_fd);
    }
    if (sockfd != -1)
    {
        close(sockfd);
    }
    if (ai != NULL)
    {
        freeaddrinfo(ai);
    }
    // TODO: change malloc to new and free
    if (pfds)
        delete[] pfds;
}

void Server::sigintHandler(int signal)
{
    if (signal == SIGINT)
    {
        printf("Ctrl- C Closing connection\n"); // Print the message.
        // TODO: should figure out a way to cleanup memory and close fd's
        //  cleanup();
        //  close(new_fd);
        //  close(sockfd);

        printf("Exiting\n"); // Print the message.
        // Memory leaks will be present, since we can't run freeaddrinfo(res) unless it is declared as a global
        exit(0);
    }
}

// int main(int argc, char *argv[])
// {
//     if (argc != 2)
//     {
//         printf("Usage: please ./a.out <port number, has to be over 1024>\n");
//         return(1);
//     }
//     try {
//         Server server(argv[1]);
//         server.start();

//     } catch (const std::exception& e) {
//         std::cerr << "Server error: " << e.what() <<std::endl;
//         return 1;
//     }
//     return (0);
// }
