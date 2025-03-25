#ifndef PARSER_HPP
#define PARSER_HPP

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

#define DEFAULT_CONFIG "./server/default.conf"
#define BUFFER_SIZE 1000
#define INIT_FD_SIZE 2
#define END_HEADER "\r\n\r\n"

enum ParseKeyValueResult
{
    INVALID,
    KEY_VALUE_PAIR,
    KEY_VALUE_PAIR_WITH_QUOTES,
    KEY_ARRAY_PAIR,
    EMPTY_LINE
};

class Parser
{

private:
    std::vector<Server> servers; // constructor calls config parser and instantiates server(s)
    std::string line;

    // Parser
    void setRootDirectory(const std::string &root_directory);
    bool parseLocationBlock(std::istream &config_file, Server &server);
    ParseKeyValueResult checkKeyPair(const std::string &line);
    bool parseKeyValue(const std::string &line, std::string &key, std::string &value);
    bool parseErrorBlock(std::istream &config_file, Server &server);
    bool parseServerBlock(std::istream &config_file, Server &server);
    bool ipValidityChecker(std::string &ip);
    int checkValidQuotes(const std::string &line);
    bool parseKeyArray(const std::string &line, std::string &key, std::set<std::string> &value);
    bool checkValidSquareBrackets(const std::string &line);
    bool checkMaxBodySize(const std::string &value);

public:
    Parser();
    ~Parser();
    std::vector<Server> parseConfig(const std::string config_file);
    int listener_fd; // Listening socket fd
    std::string port;
    std::string host;
    std::string name;
    std::string client_max_body_size;
    std::string index;
    bool server_block_ok, error_block_ok, location_bloc_ok, new_server_found;
    std::string root_directory;
    std::map<std::string, Route> routes;    // Mapping of URIs to Route objects
    std::map<int, std::string> error_pages; // Error pages mapped by status code
};

#endif