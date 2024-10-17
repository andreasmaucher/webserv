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


class Server {

private:
    int sockfd;
    int new_fd;
    struct addrinfo* res;
    
    Server(const Server&other);
    Server& operator=(const Server &other);
    // ChadGpt suggested these two lines, not sure for what
    // Server(Server&& other);
    // Server& operator=(Server&& other);
    void Server::setup(const std::string& port);
    void Server::cleanup();


public:
    Server(const std::string &port);
    ~Server();

    void Server::start();
    void Server::handleSiging(int signal);
    static void Server::sigintHandler(int signal);
};