/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cestevez <cestevez@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2024/12/13 15:03:26 by cestevez         ###   ########.fr       */
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
#include "httpRequest.hpp"
#include "requestParser.hpp"
#include "httpResponse.hpp"
#include "responseHandler.hpp"
#include "serverConfig.hpp"
#include "../tests/testsHeader.hpp"

#define MAX_SIM_CONN 10
#define BUFFER_SIZE 100
#define PORT "8080"
#define INIT_FD_SIZE 2
#define END_HEADER "\r\n\r\n"


class Server {

private:
    ServerConfig config;
    //std::vector <serverConfig> servers;
    std::vector<pollfd> pfds_vec;
    
    //std::map <serverConfig.server_name, pollfd> pfds_map;
    
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
    
    Server(const Server&other);
    Server& operator=(const Server &other);

    std::vector<HttpRequest> httpRequests;
    int setup(const std::string& port);
    void cleanup();
    int get_listener_socket(const std::string port);
    void *get_in_addr(struct sockaddr *sa);
    void add_to_pfds_vec(int newfd);
    void del_from_pfds_vec(int fd);

public:
    Server(const std::string &port);
    ~Server();

    int         start();
    void        receiveRequest(int i);
    void        sendResponse(int i, HttpRequest &request);
    void        new_connection();
    void        closeConnection(int &fd, int &i, std::vector<HttpRequest> &httpRequests);
    void        handleSigint(int signal);
    static void sigintHandler(int signal);
};
