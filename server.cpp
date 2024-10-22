/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizakov <mrizakov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/20 18:23:17 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/22 21:40:52 by mrizakov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include"server.hpp"


void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {  // Check if using IPv4
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    // Else return IPv6 address
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
    int listener;     // Listening socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR, to allow immediate reuse of socket after close
    int addrinfo_status; // Return status of getaddrinfo()

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Unspecified IPv4 or IPv6, can use both
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // Fill in the IP address of the server automatically when getaddrinfo(), for listening sockets
    // getaddrinfo(NULL, PORT, &hints, &ai)
    // NULL - hostname or IP address of the server, NULL means any available network, use for incoming connections
    // PORT - port
    // hints - criteria to resolve the ip address
    // ai - pointer to a linked list of results returned by getaddrinfo(). 
    // It holds the resolved network addresses that match the criteria specified in hints.
    
    
    if ((addrinfo_status = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(addrinfo_status));
        exit(1);
    }
    // gai_strerror(addrinfo_status) - use this instead of perror() or strerror() since it is designed to describe getaddrinfo() or other network function error codes
    
    // TODO: fixing this block of code in c++
    for(p = ai; p != NULL; p = p->ai_next) { //loop through the ai ll and try to create a socket 
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        // tries to bind the socket, otherwise closes it
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
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

    // Listening
    // Starts listening for incoming connections on the listener socket, with a maximum backlog of 10 connections waiting to be accepted.
    if (listen(listener, 10) == -1) {
        return -1;
    }
    // returns fd of the listening socket
    return listener;
}

// Add a new fd to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{   
    // pfds - struct of fd's
    // newfd - fd to add
    // fd_count - fd currently being used
    // fd_size - size of allocated capacity of the pollfd struct
    // If the pollfd struct doesnt have space, add more space
    if (*fd_count == *fd_size) { // if struct full, add space
        *fd_size *=2; //Double the size
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }
    (*pfds)[*fd_count].fd = newfd; // add new fd at position fd_count of pfds struct
    (*pfds)[*fd_count].events = POLLIN; //Set the fd for reading
    
    (*fd_count)++; // add fd_count to keep track of used fd's in pfds struct
}

// Removing and index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // pfds - struct of fd's
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

// Server::Server(Server&& other) noexcept: sockfd(other.ss)

Server::~Server() {
    Server::cleanup();
}

int Server::setup(const std::string& port) {
    
    struct addrinfo hints, *ai, *p;
    int             listener_fd;     // Listening socket descriptor
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (addrinfo_status = getaddrinfo(NULL, port.c_str(), &hints, &ai) != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(addrinfo_status) << std::endl;
        exit(1);
    }

    // TODO: fixing this block of code in c++

    for(p = ai; p != NULL; p = p->ai_next) { //loop through the ai ll and try to create a socket 

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            perror("Failed to create socket");
            exit(1);
        }

        int opt = 1;
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            perror("Setsockopt to reuse ports failed");
            exit(1);
        }

        if (bind(listener_fd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("Failed to bind");
            exit(1);
        }
    
    }
    if (p == NULL) {
        return (-1);
    }
    // Cleaning up ai ll
    freeaddrinfo(ai); // All done with this
    

    

    if (listen(listener_fd, SIMULTANEOUS_CON) == -1) {
        perror("Failed to listen");
        return(-1);
    }
    return(listener_fd);
}



void Server::start() {
    struct sockaddr_storage addr;
    socklen_t               addr_size = sizeof addr;
    char                    buffer[BUFF_SIZE];
    int bytes_received;


    std::cout << "Server: waiting for connections..." << std::endl;
    new_fd = accept(sockfd, (struct sockaddr*)&addr, &addr_size);
    if (new_fd == -1) {
        perror("Failed to accept connection");
        exit(1);
    }
    std::cout << "Server: connection accepted..." << std::endl;

    while ((bytes_received = recv(new_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0';
        std::cout << "Received: " << buffer << std::endl;

        if (strncmp(buffer, "close", 5) == 0) {
            std::cout << "Closing connection..." << std::endl;
            close(new_fd);
            return;
        }

        send(new_fd, "Server sent back: ", 19, 0);
        send(new_fd, buffer, strlen(buffer), 0);
    }

    if (bytes_received == 0) {
        std::cout << "Connection closed by client" << std::endl;
    } else if (bytes_received == -1) {
        perror("recv error");
    }
}

void Server::cleanup() {
    if (new_fd != -1) {
        close(new_fd);
    }
    if (sockfd != -1) {
        close(sockfd);
    }
    if (res!= NULL) {
        freeaddrinfo(res);
    }
}

void Server::sigintHandler(int signal)
{
    if (signal == SIGINT)
    {
        printf("Ctrl- C Closing connection\n"); // Print the message.
        //TODO: should figure out a way to cleanup memory
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
