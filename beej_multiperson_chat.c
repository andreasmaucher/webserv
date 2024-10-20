/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   beej_multiperson_chat.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizakov <mrizakov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/20 20:10:43 by mrizakov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define PORT "9036"

// add signals so can reuse the port
// add msgs of which client sent what
// add msg confirmation to send back to clients
// add clear msgs of what is happening - ex. added sockets and fd'
// add cleanup on exit



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
    // for(p = ai; p != NULL; p = p->ai_next) { //loop through the ai ll and try to create a socket 
    //     listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    //     if (listener < 0) { 
    //         continue;
    //     }
        
    //     // Lose the pesky "address already in use" error message
    //     setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    //     // tries to bind the socket, otherwise closes it
    //     if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
    //         close(listener);
    //         continue;
    //     }
    //     // breaks loop on success
    //     break;
    // }

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

int main(void)
{
    int listener_fd; //Listening socket fd
    int new_fd; // Newly accepted fd
    struct sockaddr_storage remoteaddr; // Client address (both IPv4 and IPv6)
    socklen_t addrlen;
    char buf[256]; //Buff for client data
    char remoteIP[INET6_ADDRSTRLEN]; //To store IP address in string form
    int fd_count = 0; // Current number of used fds
    int fd_size = 5; // Initial size of struct to hold all fds
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    listener_fd = get_listener_socket();
    if (listener_fd == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    pfds[0].fd = listener_fd;
    pfds[0].events = POLLIN; //Watch for incoming data on the listener
    fd_count = 1; // There is only 1 listener on the socket


    // Manually added option to allow to reuse ports straight after closing the server - SO_REUSEADDR
    // If this option is not configured, ports will be in TIME_WAIT state even after they are closed and can be used only after they timeout
    int opt = 1;
    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }
    
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

                        // sending a msg "close" from the client closes the connection

                        if (strncmp(buf, "close", 5) == 0) 
                        {
                            printf("Received: %s\n", buf); // Print the message.
                            printf("Closing connection\n");   // Print the message.
                            close(new_fd);
                            close(listener_fd);
                            // freeaddrinfo(res); // free the linked list
                            return 0;
                        }
                        for (int j = 0; j < fd_count; j++) {
                            
                            // Send to everyone!
                            int dest_fd = pfds[j].fd;
                            // Except the listener and ourselves
                            if (dest_fd != listener_fd && dest_fd != sender_fd) {
                                if (send(dest_fd, buf, nbytes, 0) == -1) {
                                    perror("send");
                                }
                            }
                        }
                    }
                    
                } // END handle data from client    
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
     } // END  while(1) loop 


    return 0;
}

