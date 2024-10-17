#include"server.hpp"

Server::Server(const std::string& port) : sockfd(-1), new_fd(-1), res(nullptr) {
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

    int status = getaddrinfo(nullptr, port.c_str(), &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(0);
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

    if (bind)

}