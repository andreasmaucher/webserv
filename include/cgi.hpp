#ifndef CGI_HPP
#define CGI_HPP

#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <algorithm>
#include <limits.h>
#include "../include/httpRequest.hpp"
#include "../include/requestParser.hpp"
#include "../include/httpResponse.hpp"

class HttpResponse;

class CGI
{
public:
    CGI();
    CGI(int clientSocket, const std::string &scriptPath, const std::string &method,
        const std::string &queryString, const std::string &requestBody);

    void handleCGIRequest(int &fd, HttpRequest &request, HttpResponse &response);

    static bool isCGIRequest(const std::string &path);

    void sendResponse(const std::string &response) const;

private:
    int clientSocket;
    std::string scriptPath;
    std::string method;
    std::string queryString;
    std::string requestBody;

    void setCGIEnvironment(const HttpRequest &httpRequest) const;
    std::string executeCGI(int &fd, HttpResponse &response);
    void sendCGIOutputToClient(int pipefd) const;
    void sendHttpResponseHeaders(const std::string &contentType) const;
    std::string resolveCGIPath(const std::string &uri);
};

#endif