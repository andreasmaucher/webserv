#ifndef CGI_HPP
#define CGI_HPP

#include <cstdlib>  // For setenv()
#include <unistd.h>    // For fork(), exec()
#include <sys/wait.h>  // For wait()
#include <fcntl.h>     // For file descriptor control
#include <iostream>
#include <sys/socket.h> // For send()
#include <string.h>    // For memset
#include <sstream>  // For std::stringstream

class CGI 
{
    public:
        CGI(int clientSocket, const std::string& scriptPath, const std::string& method,
            const std::string& queryString, const std::string& requestBody);

        void handleCGIRequest();

        static bool isCGIRequest(const std::string& path);

        void sendResponse(const std::string& response) const;

    private:
        int clientSocket;
        std::string scriptPath;
        std::string method;
        std::string queryString;
        std::string requestBody;

        void setCGIEnvironment() const;
        std::string executeCGI();
        void sendCGIOutputToClient(int pipefd) const;
        void sendHttpResponseHeaders(const std::string& contentType) const;
        std::string readFromPipe(int pipefd) const;
};

#endif