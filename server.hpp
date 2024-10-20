/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizakov <mrizakov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/20 18:23:13 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/20 19:44:37 by mrizakov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <csignal>
#include <cerrno>

#define SIMULTANEOUS_CON 10
#define BUFF_SIZE 512


class Server {

private:
    int sockfd;
    int new_fd;
    struct addrinfo* res;
    
    Server(const Server&other);
    Server& operator=(const Server &other);
    // TODO: add canonical orthodox form if any of it is missing
    // ChadGpt suggested these two lines, not sure for what
    // Server(Server&& other);
    // Server& operator=(Server&& other);
    void setup(const std::string& port);
    void cleanup();


public:
    Server(const std::string &port);
    ~Server();

    void        start();
    void        handleSigint(int signal);
    static void sigintHandler(int signal);
};