/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490"  // the port the server will be listening on
#define BACKLOG 10    // how many pending connections queue will hold
#define MESSAGE "Hello from the server!\n"  // message to send to the client
// name of a file as parameter then it runs all the tests of the file
// when we send it over we could chunk it, how is the size of the buffer defined?
// test:: not sending the null terminator
// test the polling with several clients
// run a script that runs several clients at the same time
// output error messages to a logfile (what message, parameters, error code etc.)
// for each client a separate logfile
// how can we identify clients 
// later udp version as well

// Function prototype for get_in_addr
void *get_in_addr(struct sockaddr *sa);

int main(void) {
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    int rv;
    struct sockaddr_storage their_addr;  // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    // Set up hints for address info
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // Use my IP

    // Get address info
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break; // Successfully bound
    }

    freeaddrinfo(servinfo); // All done with this structure

    // Check if we successfully bound a socket
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    // Listen for incoming connections
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    // Continuous loop to accept connections
    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue; // Continue to accept other connections
        }

        // Get the IP address of the connected client
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        // Send a message to the client
        if (send(new_fd, MESSAGE, strlen(MESSAGE), 0) == -1) {
            perror("send");
        }

        close(new_fd); // Close the connection to the client
    }

    close(sockfd); // Close the listening socket (this line is unreachable)
    return 0;
}

// Utility function to get the appropriate address
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
