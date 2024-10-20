/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizakov <mrizakov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/20 18:23:17 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/20 19:41:44 by mrizakov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include"server.hpp"

Server::Server(const std::string& port) : sockfd(-1), new_fd(-1), res(NULL) {
    signal(SIGINT, sigintHandler);
    setup(port);
}

// Server::Server(Server&& other) noexcept: sockfd(other.ss)

Server::~Server() {
    Server::cleanup();
}

void Server::setup(const std::string& port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        exit(1);
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        perror("Failed to create socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("Setsockopt to reuse ports failed");
        exit(1);
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Failed to bind");
        exit(1);
    }

    if (listen(sockfd, SIMULTANEOUS_CON) == -1) {
        perror("Failed to listen");
        exit(1);
    }

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

int main(int argc, char *argv[]) {
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
