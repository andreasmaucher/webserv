/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cestevez <cestevez@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/30 12:33:16 by cestevez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

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
// void Server::add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
// {
//     // If the pollfd struct doesnt have space, add more space
//     if (*fd_count == *fd_size)
//     {                  // if struct full, add space
//         *fd_size *= 2; // Double the size

//         // TODO: switch  realloc to resize and use vectors instead of array for pfds
//         *pfds = (struct pollfd *)realloc(*pfds, sizeof(**pfds) * (*fd_size));
//     }
//     (*pfds)[*fd_count].fd = newfd;                // add new fd at position fd_count of pfds struct
//     (*pfds)[*fd_count].events = POLLIN | POLLOUT; // Set the fd for reading

//     (*fd_count)++; // add fd_count to keep track of used fd's in pfds struct
// }

void Server::add_to_pfds_vec(int newfd)
{
    struct pollfd new_pollfd;
    new_pollfd.fd = newfd;
    new_pollfd.events = POLLIN | POLLOUT;
    new_pollfd.revents = 0;

    pfds_vec.push_back(new_pollfd);
    fd_count++;
    
    // pfds_vec[pfds_vec.size()];
    
    // // If the pollfd struct doesnt have space, add more space
    // if (*fd_count == *fd_size)
    // {                  // if struct full, add space
    //     *fd_size *= 2; // Double the size

    //     // TODO: switch  realloc to resize and use vectors instead of array for pfds
    //     *pfds = (struct pollfd *)realloc(*pfds, sizeof(**pfds) * (*fd_size));
    // }
    // (*pfds)[*fd_count].fd = newfd;                // add new fd at position fd_count of pfds struct
    // (*pfds)[*fd_count].events = POLLIN | POLLOUT; // Set the fd for reading

    // (*fd_count)++; // add fd_count to keep track of used fd's in pfds struct
}

// Removing and index from the set
void Server::del_from_pfds_vec(int fd)
{
    for (size_t i = 0; i < pfds_vec.size(); i++)
    {
        if (pfds_vec[i].fd == fd)
        {
            send(pfds_vec[i].fd, "Closing connection. Goodbye!", 29, 0);
            close(pfds_vec[i].fd);
            std::cout << "Closing connection on fd: " << pfds_vec[i].fd << " index: " << i << " of pfds vector" << std::endl;
            pfds_vec.erase(pfds_vec.begin() + i);

        }

    }
    fd_count--;
    // i - index of fd to be removed
    // fd_count - pointer to number of fd's being monitored
    // pfds[i] = pfds[*fd_count - 1]; // replace the fd to be removed (fd[i]) with the last element of array
    // (*fd_count)--;                 // make the size of the array smaller by one
    // // Not doing realloc() to change the actual size of the array, since this can be slow
}



void Server::del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // i - index of fd to be removed
    // fd_count - pointer to number of fd's being monitored
    pfds[i] = pfds[*fd_count - 1]; // replace the fd to be removed (fd[i]) with the last element of array
    (*fd_count)--;                 // make the size of the array smaller by one
    // Not doing realloc() to change the actual size of the array, since this can be slow
}

Server::Server(const std::string &port) : sockfd(-1), pfds_vec(1), new_fd(-1) 
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

    // std::vector<pollfd> pfds_vec(1);

    HttpRequest newRequest;
    // pollfd pfd;
    // pfd.fd = 0;
    // pfds_vec[0].events = POLLIN | POLLOUT;
    // pfds_vec.push_back(pfd);
    httpRequests.push_back(newRequest);
    std::cout << "httpRequest vector size in setup: " << httpRequests.size() << std::endl;
    // for (size_t i = 0; i < pfds_vec.size(); i++) {
    //     pollfd pfd;
    //     pfd.fd = i;
    //     pfd.events = POLLIN | POLLOUT;
    //     pfds_vec.push_back(pfd);
    //     httpRequests.push_back(newRequest);
    // }
    // std::vector<pollfd> pfds_vec(INIT_FD_SIZE);

    // HttpRequest newRequest;
    // for (size_t i = 0; i < pfds_vec.size(); i++) {
    //     pollfd pfd;
    //     pfd.fd = i;
    //     pfd.events = POLLIN | POLLOUT;
    //     pfds_vec.push_back(pfd);
    //     httpRequests.push_back(newRequest);
    // }
    request_fully_received = 0;
    response_ready_to_send = 0;
    

    // TODO: change to vector
    // pfds = new struct pollfd[fd_size]();

    listener_fd = get_listener_socket(port);
    if (listener_fd == -1)
    {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    pfds_vec[0].fd = listener_fd;
    pfds_vec[0].events = POLLIN | POLLOUT;
    pfds_vec[0].revents = 0;
                 // Watch for incoming data on the listener
    fd_count = 1; // There is only 1 listener on the socket
    // printf("in setup () pfds_vec.size() %lu\n", pfds_vec.size());
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
        // printf("in start()  pfds_vec.size() %lu\n", pfds_vec.size());
        // printf("fd_count is %i\n", fd_count);
        poll_count = poll(pfds_vec.data(), pfds_vec.size(), -1);
        // printf("in start()  pfds_vec.size() %lu\n", pfds_vec.size());

        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }

        // Run through the existing connections looking for data to read
        for (size_t i = 0; i < pfds_vec.size(); i++)
        {
            // printf("in main loop  pfds_vec.size() %lu\n", pfds_vec.size());

            // printf("i is %li\n", i);
            // printf("pfds_vec.size() is %li\n", pfds_vec.size());


            if (pfds_vec[i].revents & POLLIN)
            {
                std::cout << "i is " << i << " in POLLIN" << std::endl;
                if (pfds_vec[i].fd == listener_fd)
                {
                    printf("\nNew connection!\n");
                    new_connection(i);
                } else {
                    printf("Processing Request!\n");
                    request(i);
                }
            }
            // Received a connection
            if (pfds_vec[i].revents & POLLOUT)
            {
                std::cout << "i is " << i << " in POLLOUT" << std::endl;
                // printf("Response!\n");
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
        add_to_pfds_vec(new_fd);
        // add_to_pfds(&pfd_, new_fd, &fd_count, &fd_size);
        std::cout << "Added new fd to pollfd struct, new fd is: " << new_fd << std::endl;
        HttpRequest newRequest;             // Assuming HttpRequest has a default constructor
        httpRequests.push_back(newRequest); // Add it to the vector
        std::cout << "httpRequest vector size: " << httpRequests.size() << std::endl;
        //httpRequests[i].complete = 0; // is initiated to false in the constructor already!
        printf("pollserver: new connection from %s on socket %d\n",
               inet_ntop(remoteaddr.ss_family,
                         get_in_addr((struct sockaddr *)&remoteaddr),
                         remoteIP, INET6_ADDRSTRLEN),
               new_fd);
    }
}

void Server::request(int i)
{
    std::cout << "Request function called for client i:" << i << ". Accessing request with same index" << std::endl;
    if (!httpRequests[i].complete)
    {
        // If not the listener, we're just the regular client
        // printf("At recv fn \n");
        int nbytes = recv(pfds_vec[i].fd, buf, sizeof buf, 0);
        int sender_fd = pfds_vec[i].fd;
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
            close(pfds_vec[i].fd); // Closing fd
            // del_from_pfds(pfds, i, &fd_count);
            std::cout << "Deleting client i:" << i << " from pfds vector" << std::endl;
            httpRequests.erase(httpRequests.begin() + i); // Erase the HttpRequest for this fd
            std::cout << "Erased request i:" << i << " from httpRequests vector" << std::endl;
        }
        else
        {
            // Received some good data from the client
            // Printing out received data
            buf[nbytes] = '\0';
            httpRequests[i].raw_request.append(buf);
            if (httpRequests[i].raw_request.find(END_HEADER) != std::string::npos)
            {
                //std::cout << "Full request from client " << i << " is: " << httpRequests[i].raw_request << std::endl;
                //std::cout << "Found end of request command \"close\", stopped reading request " << std::endl;
                // PARSER COMES HERE
                RequestParser::parseRawRequest(httpRequests[i]);
            }
        } // END handle data from client
    } // END got ready-to-read from poll()
}

void Server::response(int i)
{
    std::cout << "Response function called for client i:" << i << ". Accessing request with same index" << std::endl;
    if (httpRequests[i].complete)
    {
        //HttpResponse response; // (or instantiate at the same time as HttpRequest)
        //ResponseHandler::processRequest(httprequests[i],response); // processes the request and creates a response
        //from this point the response is ready, you can access it calling response.generateResponseStr(), which resturns the whole response as a single std::string for sending

        std::string response =
            "HTTP/1.1 200 OK\r\nDate: Fri, 27 Oct 2023 14:30:00 GMT\r\nServer: CustomServer/1.0\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nConnection: keep-alive\r\n\r\nHello, World!\r\n";
        // TODO: RESPONSE GOES HERE
        if (send(pfds_vec[i].fd, response.c_str(), response.size(), 0) == -1)
        {
            perror("send");
        }
        std::cout << "Response sent to client " << i << " after receiving following request: " << std::endl;
        httpRequests[i].printRequest();
        httpRequests[i].reset(); // cleans up the request
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
