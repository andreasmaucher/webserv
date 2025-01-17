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
#include "../include/httpRequest.hpp"
#include "../include/requestParser.hpp"

class CGI 
{
    public:
        CGI();
        CGI(int clientSocket, const std::string& scriptPath, const std::string& method,
            const std::string& queryString, const std::string& requestBody);

        void handleCGIRequest(HttpRequest& request);

        static bool isCGIRequest(const std::string& path);

        void sendResponse(const std::string& response) const;

    private:
        int clientSocket;
        std::string scriptPath;
        std::string method;
        std::string queryString;
        std::string requestBody;

        void setCGIEnvironment(const HttpRequest& httpRequest) const;
        std::string executeCGI();
        void sendCGIOutputToClient(int pipefd) const;
        void sendHttpResponseHeaders(const std::string& contentType) const;
        std::string resolveCGIPath(const std::string& uri);
};

#endif