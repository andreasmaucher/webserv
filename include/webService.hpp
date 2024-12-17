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
#define MAX_SIM_CONN 10
#define BUFFER_SIZE 100
#define PORT "8080"
#define INIT_FD_SIZE 2
#define END_HEADER "\r\n\r\n"


class WebService {

private:
    std::vector <Server> servers; // constructor calls config parser and instantiates server(s)
    std::vector<pollfd> pfds_vec;
    
    std::vector<HttpRequest> httpRequests;
//    std::map <Server, pollfd> server_pfds_map;
    std::map <int, Server*> pfd_to_server;   //pollfd mapped to its server object
    std::map <int, HttpRequest*> pfd_to_request;

    struct sockaddr_storage remoteaddr; // Client address (both IPv4 and IPv6)
    socklen_t addrlen;
    char buf[BUFFER_SIZE]; //Buff for client data
    char remoteIP[INET6_ADDRSTRLEN]; //To store IP address in string form
    
    int reuse_socket_opt;
    int addrinfo_status; // Return status of getaddrinfo()
    //int poll_count;

    struct addrinfo hints, *ai, *p;
    
    int setupSockets();
    void parseConfig(const std::string &config_file);
    void cleanup();
    int get_listener_socket(const std::string port);
    void *get_in_addr(struct sockaddr *sa);
    void addToPfds(int newfd);
    void del_from_pfds_vec(int fd);

public:
    WebService(const std::string &config_file);
    ~WebService();

    int         start();
    void        receiveRequest(int i);
    void        sendResponse(int i, HttpRequest &request);
    void        newConnection();
    void        closeConnection(int &fd, int &i, std::vector<HttpRequest> &httpRequests);
    void        handleSigint(int signal);
    static void sigintHandler(int signal);
};