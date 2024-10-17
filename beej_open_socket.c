/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   beej_open_socket.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/07 14:07:00 by mrizhakov        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>


// Program which simulates a minimalistic server, capable of accepting 1 connection
// Usage: launch with one parameter specifying the port to open : ./a.out 2351
// Usage: open a separate terminal and launch netcat: nc -v localhost 2351
// Usage: send msgs from client/netcat, which will be printed out on server.
// Usage: type "close" on client when done to close connection and exit server

#define MESSAGE "Hello from the server!\n"  //
#define BACKLOG 10 // how many pending connections queue will hold

int new_fd;
int sockfd;

void handle_sigint(int signal)
{
    if (signal == SIGINT)
    {
        printf("Ctrl- C Closing connection\n"); // Print the message.
        close(new_fd);
        close(sockfd);
        printf("Exiting\n"); // Print the message.
        // Memory leaks will be present, since we can't run freeaddrinfo(res) unless it is declared as a global
        exit(1);
    }
}

// NEW FUNCTION: Sends file contents to the client
void send_file_contents(int client_fd, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        const char *response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        return;
    }
    printf("Sending file contents...\n"); // Debug line

    char buffer[1024];
    size_t bytes_read;
    const char *response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
    send(client_fd, response_header, strlen(response_header), 0);

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_fd, buffer, bytes_read, 0);
    }
    printf("Finished sending file contents.\n"); // Debug line
    fclose(file);
}

int main(int argc, char *argv[])
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    char buffer[1024];
    int bytes_received;
    if (argc != 2)
    {
        printf("Usage: please ./a.out <port number, has to be over 1024>\n");
        exit(1);
    }
    signal(SIGINT, handle_sigint);

    // !! don't forget your error checking for these calls !!

    // Step 1. first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    getaddrinfo(NULL, argv[1], &hints, &res);

    // Step 2. make a socket, bind it, and listen on it:
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // Step 2.1. Manually added option to allow to reuse ports straight after closing the server - SO_REUSEADDR
    // If this option is not configured, ports will be in TIME_WAIT state even after they are closed and can be used only after they timeout
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("Failed to bind socket to port");
        return (errno);
    }
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("Failed to listen(), maybe number of connections exceeds allowed incoming queue");
        return (errno);
    }

    // now accept an incoming connection:
    printf("Server: waiting for connections...\n");
    addr_size = sizeof their_addr;
    //new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    while ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) != -1) {
        printf("Server: accepted...\n");
        
        // NEW LINE: Call the function to send file contents
        send_file_contents(new_fd, "message.txt"); // Change "message.txt" to your desired file

        close(new_fd); // Close connection after response
    }
    // Blocking
    // A very simplistic way of making a socket non-blocking would be to change it's parameters using fcntl()
    // By default fd's are blocking, but we can instruct the kernel to make them non-blocking
    // The downside of this approach is that this method will use a lot of CPU time,
    // since this socket it is constantly busy-wait looking for data on the socket
    if (new_fd == -1)
    {
        perror("Failed to open a socket fd for this connection");
        return (errno);
    }
    printf("Server: accepted...\n");
    // Step 3. Receive msgs
    while ((bytes_received = recv(new_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0';    // Null-terminate the buffer.
        printf("Received: %s\n", buffer); // Print the message.

        // NEW LINE: Respond back to the client
        const char *response = "Server received your message.\n"; 
        send(new_fd, response, strlen(response), 0); // NEW LINE

        // sending a msg "close" from the client closes the connection
        if (strncmp(buffer, "close", 5) == 0) 
        {
            printf("Received: %s\n", buffer); // Print the message.
            printf("Closing connection\n");   // Print the message.
            close(new_fd);
            close(sockfd);
            freeaddrinfo(res); // free the linked list
            return 0;
        }
        // Step 4. Sending back a msg to the client
        send(new_fd, "Server sent back: ", 19, 0);

        send(new_fd, buffer, strlen(buffer), 0);
    }
    // if 0 bytes are received, the client has closed the connection from it's side

    if (bytes_received == 0)
    {
        printf("Connection closed by client.\n");
    }
    else if (bytes_received == -1)
    {
        perror("recv");
    }

    // Step 5: Close the connection to this client.
    close(new_fd);
    close(sockfd);
    freeaddrinfo(res); // free the linked list
    return 0;
}
