/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/10/29 15:48:23 by mrizhakov        ###   ########.fr       */
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
#include <poll.h>
#include <arpa/inet.h>
#include <vector>
#include <cstdio>
#include "../http/httpRequest.hpp"
#include "../http/requestParser.hpp"


#define MAX_SIM_CONN 10
#define BUFFER_SIZE 512
#define PORT "8080"
#define INIT_FD_SIZE 2
// #define END_HEADER "\r\n\r\n"
#define END_HEADER "close"




// add signals so can reuse the port
// add msgs of which client sent what
// add msg confirmation to send back to clients
// add clear msgs of what is happening - ex. added sockets and fd'
// add cleanup on exit

class Server {

private:
    int sockfd;
    // pfds - struct of fd's
    // newfd - fd to add
    // fd_count - fd currently being used
    // fd_size - size of allocated capacity of the pollfd struct
    // struct pollfd *pfds;
    std::vector<pollfd> pfds_vec;
    int fd_count;
    int fd_size;
    int new_fd; // Newly accepted fd
    int listener_fd; //Listening socket fd

    struct sockaddr_storage remoteaddr; // Client address (both IPv4 and IPv6)
    socklen_t addrlen;
    char buf[BUFFER_SIZE]; //Buff for client data
    char remoteIP[INET6_ADDRSTRLEN]; //To store IP address in string form
    
    int reuse_socket_opt;
    int addrinfo_status; // Return status of getaddrinfo()
    bool request_fully_received;
    bool response_ready_to_send;
    int poll_count;

    struct addrinfo hints, *ai, *p;
    // struct addrinfo* res;
    
    Server(const Server&other);
    Server& operator=(const Server &other);
    // TODO: add canonical orthodox form if any of it is missing
    // ChadGpt suggested these two lines, not sure for what
    // Server(Server&& other);
    // Server& operator=(Server&& other);
    std::vector<HttpRequest> httpRequests;
    int setup(const std::string& port);
    void cleanup();
    int get_listener_socket(const std::string port);
    void *get_in_addr(struct sockaddr *sa);
    void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size);
    void add_to_pfds_vec(int newfd);
    void del_from_pfds_vec(int fd);

    void del_from_pfds(struct pollfd pfds[], int i, int *fd_count);





public:
    Server(const std::string &port);
    ~Server();

    int        start();
    void        request(int i);
    void        response(int i);
    void        new_connection(int i);
    void        handleSigint(int signal);
    static void sigintHandler(int signal);
};
