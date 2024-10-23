/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/24 00:07:42 by mrizhakov        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"


void * Server::get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {  // Check if using IPv4
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    // Else return IPv6 address
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int Server::get_listener_socket(std::string port)
{
    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Unspecified IPv4 or IPv6, can use both
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // Fill in the IP address of the server automatically when getaddrinfo(), for listening sockets
    reuse_socket_opt = 1;

    // getaddrinfo(NULL, PORT, &hints, &ai)
    // NULL - hostname or IP address of the server, NULL means any available network, use for incoming connections
    // PORT - port
    // hints - criteria to resolve the ip address
    // ai - pointer to a linked list of results returned by getaddrinfo(). 
    // It holds the resolved network addresses that match the criteria specified in hints.
    if ((addrinfo_status = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0) {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(addrinfo_status));
        exit(1);
    }
    // gai_strerror(addrinfo_status) - use this instead of perror() or strerror() since it is designed to describe getaddrinfo() or other network function error codes
    
    for(p = ai; p != NULL; p = p->ai_next) { //loop through the ai ll and try to create a socket 
        listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener_fd < 0) { 
            continue;
        }

        // Lose the pesky "address already in use" error message
        // Manually added option to allow to reuse ports straight after closing the server - SO_REUSEADDR

        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_socket_opt, sizeof(reuse_socket_opt)) < 0) {
            perror("setsockopt(SO_REUSEADDR) failed");
            exit(1);
        }
        // tries to bind the socket, otherwise closes it
        if (bind(listener_fd, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener_fd);
            continue;
        }
        // breaks loop on success
        break;
    }

    // If we got here, it means we didn't get bound. Reached end of ai ll
    if (p == NULL) {
        return -1;
    }
    // Cleaning up ai ll
    freeaddrinfo(ai); // All done with this
    ai = NULL;

    // Listening
    // Starts listening for incoming connections on the listener socket, with a maximum backlog of 10 connections waiting to be accepted.
    if (listen(listener_fd, MAX_SIM_CONN) == -1) {
        return -1;
    }
    // returns fd of the listening socket
    return listener_fd;
}

// Add a new fd to the set
void Server::add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{   
    // If the pollfd struct doesnt have space, add more space
    if (*fd_count == *fd_size) { // if struct full, add space
        *fd_size *=2; //Double the size
        // TODO: switch  realloc to resize and use vectors instead of array for pfds
        *pfds = (struct pollfd*)realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }
    (*pfds)[*fd_count].fd = newfd; // add new fd at position fd_count of pfds struct
    (*pfds)[*fd_count].events = POLLIN; //Set the fd for reading
    
    (*fd_count)++; // add fd_count to keep track of used fd's in pfds struct
}

// Removing and index from the set
void Server::del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // i - index of fd to be removed
    // fd_count - pointer to number of fd's being monitored
    pfds[i] =  pfds[*fd_count - 1]; // replace the fd to be removed (fd[i]) with the last element of array
    (*fd_count)--; // make the size of the array smaller by one
    // Not doing realloc() to change the actual size of the array, since this can be slow
}

Server::Server(const std::string& port) : sockfd(-1), new_fd(-1) {
    signal(SIGINT, sigintHandler);
    setup(port);
}

Server::~Server() {
    // Added cleanup to exit conditions, need to check if it is always triggered
    // TODO: Maybe worth puttting back in the destructor
    // Server::cleanup();
    std::cout << "Server exited" << std::endl;
}


int Server::setup(const std::string& port) {
    fd_count = 0; // Current number of used fds
    fd_size = INIT_FD_SIZE; // Initial size of struct to hold all fds
    //TODO: change to vector
    pfds = new struct pollfd[fd_size]();

    listener_fd = get_listener_socket(port);
    if (listener_fd == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    pfds[0].fd = listener_fd;
    pfds[0].events = POLLIN; //Watch for incoming data on the listener
    fd_count = 1; // There is only 1 listener on the socket
    return (listener_fd);
    // Step 1 END: SETUP
}

int Server::start() {
    // STEP 2 START: MAIN LOOP
    // Main loop
     while (1)
     {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }
        
        // Run through the existing connections looking for data to read
        for (int i = 0; i < fd_count; i++)
        {
            // Check if an fd is ready to read
            if (pfds[i].revents & POLLIN) { // Received a connection
                if (pfds[i].fd == listener_fd) { // If listener is ready to read, handle new connection
                    addrlen = sizeof remoteaddr;
                    new_fd = accept(listener_fd, 
                        (struct sockaddr *)&remoteaddr, 
                        &addrlen);
                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);
                        
                        printf("pollserver: new connection from %s on socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            new_fd);
                    }
                } else {
                    // If not the listener, we're just the regular client
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
                    int sender_fd = pfds[i].fd;
                    
                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("pollserver: socket %d hung up \n", sender_fd);
                        } else {
                            perror("recv");
                        }
                        close(pfds[i].fd); //Closing fd
                        del_from_pfds(pfds, i, &fd_count);

                    } else {
                        // Received some good data from the client
                        // Printing out received data
                        buf[nbytes] = '\0';
                        printf("Received: %s\n", buf);
                        // raw_requests[i] +=std::string(buf, nbytes);                           
                        // printf("EROOR HERE\n");

                        // if (raw_requests[i].find("\r\n\r\n") != std::string::npos)
                        // {
                        //     printf("OR EROOR HERE\n");

                        //     std::cout << "Full request from client " << i << " is: " << raw_requests[i] <<std::endl;
                        //     //PARSER COMES HERE
                        // }
                        // TODO: Add parser here to parse received messages

                        // sending a msg "close" from the client closes the connection

                        if (strncmp(buf, "close", 5) == 0) 
                        {
                            printf("Received: %s\n", buf); // Print the message.
                            printf("Closing connection\n");   // Print the message.
                            //TODO: this closing of FD's is doubtful, maybe need to rewrite
                            for (int j = 0; j < fd_count; j++)
                            {
                                close(pfds[j].fd);
                            }
                            cleanup();
                            return 0;
                        }
                        // Sending back simple echo message
                        send(pfds[i].fd, "Server sent back: ", 19, 0);

                        if (send(pfds[i].fd, buf, nbytes, 0) == -1) {
                                    perror("send");
                                }
                    }
                    
                } // END handle data from client    
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
     } // END  while(1) loop 

    // Step 2 END: MAIN LOOP
}

void Server::cleanup() {
    if (new_fd != -1) {
        close(new_fd);
    }
    if (sockfd != -1) {
        close(sockfd);
    }
    if (ai != NULL) {
        freeaddrinfo(ai);
    }
    //TODO: change malloc to new and free
    if (pfds)
        delete[] pfds;
}

void Server::sigintHandler(int signal)
{
    if (signal == SIGINT)
    {
        printf("Ctrl- C Closing connection\n"); // Print the message.
        //TODO: should figure out a way to cleanup memory and close fd's
        // cleanup();
        // close(new_fd);
        // close(sockfd);
        
        printf("Exiting\n"); // Print the message.
        // Memory leaks will be present, since we can't run freeaddrinfo(res) unless it is declared as a global
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: please ./a.out <port number, has to be over 1024>\n");
        return(1);
    }
    try {
        Server server(argv[1]);
        server.start();
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() <<std::endl;
        return 1;
    }
    return (0);
}

