/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   beej_open_socket.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/09/24 17:14:06 by mrizhakov        ###   ########.fr       */
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

#define MYPORT "3494" // the port users will be connecting to
#define BACKLOG 10    // how many pending connections queue will hold

int main(int argc, char *argv[])
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;
    char buffer[1024];
    int bytes_received;

    // !! don't forget your error checking for these calls !!

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    getaddrinfo(NULL, argv[1], &hints, &res);

    // make a socket, bind it, and listen on it:

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bind(sockfd, res->ai_addr, res->ai_addrlen);
    listen(sockfd, BACKLOG);

    // now accept an incoming connection:
    printf("Server: waiting for connections...\n");
    addr_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    printf("Server: accepted...\n");

    while ((bytes_received = recv(new_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0';    // Null-terminate the buffer.
        printf("Received: %s\n", buffer); // Print the message.
        if (strncmp(buffer, "close", 5) == 0)
        {
            printf("Received: %s\n", buffer); // Print the message.
            printf("Closing connection\n");   // Print the message.
            close(new_fd);
            return 0;
        }

        // Optionally, send a response back to the client.
        send(new_fd, "Message received!\n", 18, 0);
    }

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
    return 0;
}
