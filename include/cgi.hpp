#ifndef CGI_HPP
#define CGI_HPP

#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
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
#include "../include/webService.hpp"

#include <map>
#include <ctime>
#include <fcntl.h>

// this is the path to the python interpreter, needs to be adjusted depening on the users system
#define PYTHON_PATH "/usr/bin/python3";

#define CGI_TIMEOUT 5

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

    static void checkRunningProcesses();

private:
    int clientSocket;
    std::string scriptPath;
    std::string method;
    std::string queryString;
    std::string requestBody;

    char **setCGIEnvironment(const HttpRequest &httpRequest) const;
    void executeCGI(int &fd, HttpResponse &response, HttpRequest &request);
    void sendCGIOutputToClient(int pipefd) const;
    void sendHttpResponseHeaders(const std::string &contentType) const;
    std::string resolveCGIPath(const std::string &uri);
    static std::string constructErrorResponse(int status_code, const std::string &message);

    struct CGIProcess
    {
        time_t start_time; // When the process started
        int output_pipe;   // Only need output pipe to read data
        int response_fd;
        HttpRequest *request; // To update request status if timeout occurs
        HttpResponse *response;

        CGIProcess() : start_time(0), output_pipe(-1), request(NULL) {}
    };

    static std::map<pid_t, CGIProcess> running_processes;

    static void addProcess(pid_t pid, int output_pipe, int response_fd, HttpRequest *req, HttpResponse *response);
    static void cleanupProcess(pid_t pid);
    static void readAndSendCGIResponse(pid_t pid, CGIProcess proc);
    std::string extractPathInfo(const std::string &uri);
    std::string getStatusMessage(int status_code);
    pid_t runChildCGI(int pipe_in[2], int pipe_out[2], HttpRequest &request);

    void postRequest(int pipe_in[2]);
    static bool isfdOpen(int fd);
};

#endif