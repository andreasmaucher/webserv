#pragma once

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
#include "debug.hpp"

#define DEFAULT_CONFIG "tomldb.config"
#define MAX_BACKLOG_UNACCEPTED_CON 200
#define BUFFER_SIZE 1000
// #define PORT "8080"
#define INIT_FD_SIZE 2
#define END_HEADER "\r\n\r\n"
#define MAX_CGI_BODY_SIZE 1000000

class WebService
{

private:
    static std::vector<Server> servers; // constructor calls config parser and instantiates server(s)

    // std::unordered_map<int, std::pair<Server*, HttpRequest*>> pfd_to_server_request; //client_fds to server and request objectient address (both IPv4 and IPv6)
    socklen_t addrlen;
    char buf[BUFFER_SIZE];           // Buff for client data
    char remoteIP[INET6_ADDRSTRLEN]; // To store IP address in string form

    int reuse_socket_opt;
    int addrinfo_status; // Return status of getaddrinfo()
    // int poll_count;

    struct addrinfo hints, *ai, *p;
    size_t poll_start_offset;

    // std::vector <Server>  parseConfig(const std::string &config_file);
    void setupSockets();
    void mapFdToServer(int new_fd, Server &server);
    // void addToPfdsVector(int new_fd);
    void createRequestObject(int new_fd, Server &server);
    int get_listener_socket(const std::string &port);
    void *get_in_addr(struct sockaddr *sa);
    static void deleteFromPfdsVec(int &fd, size_t &i);

    // Parser
    // bool parseConfigFile(const std::string &config_filename);

public:
    WebService(const std::string &config_file);
    ~WebService();

    int start();
    void receiveRequest(int &fd, size_t &i, Server &server);
    static void sendResponse(int &fd, size_t &i, Server &server);
    void newConnection(Server &server);
    static void closeConnection(const int &fd, size_t &i, Server &server);
    void handleSigint(int signal);
    static void sigintHandler(int signal);
    static int addToPfdsVector(int new_fd, bool isCGIOutput = false);
    // static void deleteFromPfdsVec(int &fd, size_t &i);
    static void deleteFromPfdsVecForCGI(const int &fd);
    static void deleteRequestObject(const int &fd, Server &server);
    static void setPollfdEventsToOut(int fd);
    static void setPollfdEventsToIn(int fd);

    static void printPollFdStatus(pollfd *pollfd);
    static struct pollfd *findPollFd(int fd);
    static void printPollFds();
    static void setPollfdEvents(int fd, short events);
    static std::string checkPollfdEvents(int fd);

    static std::map<int, HttpResponse *> cgi_fd_to_http_response; // fds to respective server objects pointer
    static std::vector<pollfd> pfds_vec;
    static std::map<int, Server *> fd_to_server; // fds to respective server objects pointer
    static void cleanup();
                                             // all pfds (listener and client) for all servers
};