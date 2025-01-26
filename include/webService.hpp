#ifndef WEBSERVICE_HPP
#define WEBSERVICE_HPP

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
#include "server.hpp"
#include "../tests/testsHeader.hpp"

#define DEFAULT_CONFIG "./server/default.conf"
#define MAX_BACKLOG_UNACCEPTED_CON 260
#define BUFFER_SIZE 100
// #define PORT "8080"
#define INIT_FD_SIZE 2
#define END_HEADER "\r\n\r\n"

class WebService
{

private:
    static std::vector<Server> servers; // constructor calls config parser and instantiates server(s)

    static std::vector<pollfd> pfds_vec; // all pfds (listener and client) for all servers

    std::map<int, Server *> fd_to_server; // fds to respective server objects pointer
    // std::unordered_map<int, std::pair<Server*, HttpRequest*>> pfd_to_server_request; //client_fds to server and request objects

    struct sockaddr_storage remoteaddr; // Client address (both IPv4 and IPv6)
    socklen_t addrlen;
    char buf[BUFFER_SIZE];           // Buff for client data
    char remoteIP[INET6_ADDRSTRLEN]; // To store IP address in string form

    int reuse_socket_opt;
    int addrinfo_status; // Return status of getaddrinfo()
    // int poll_count;

    struct addrinfo hints, *ai, *p;

    // std::vector <Server>  parseConfig(const std::string &config_file);
    void setupSockets();
    void mapFdToServer(int new_fd, Server &server);
    void addToPfdsVector(int new_fd);
    void createRequestObject(int new_fd, Server &server);
    static void cleanup();
    int get_listener_socket(const std::string &port);
    void *get_in_addr(struct sockaddr *sa);
    void deleteFromPfdsVec(int &fd, size_t &i);
    void deleteRequestObject(int &fd, Server &server);

    // Parser
    // bool parseConfigFile(const std::string &config_filename);

public:
    WebService(const std::string &config_file);
    ~WebService();

    int start();
    void receiveRequest(int &fd, size_t &i, Server &server);
    void sendResponse(int &fd, size_t &i, Server &server);
    void newConnection(Server &server);
    void closeConnection(int &fd, size_t &i, Server &server);
    void handleSigint(int signal);
    static void sigintHandler(int signal);
};

#endif