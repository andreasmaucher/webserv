/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webService.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/02/05 22:35:40 by mrizhakov        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/webService.hpp"
#include "../../include/Parser.hpp"
#include "../../include/cgi.hpp"

std::vector<pollfd> WebService::pfds_vec;
std::vector<Server> WebService::servers;
std::map<int, Server *> WebService::fd_to_server;
std::map<int, HttpResponse *> WebService::cgi_fd_to_http_response; // fds to respective server objects pointer

WebService::WebService(const std::string &config_file)
{
    signal(SIGINT, sigintHandler);
    Parser parser;
    servers = parser.parseConfig(config_file);
    DEBUG_MSG("Configured servers", servers.size());

    for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
    {
        (*it).debugServer();
        (*it).debugPrintRoutes();
    }
    setupSockets();
}

void WebService::cleanup()
{
    for (size_t i = 0; i < pfds_vec.size(); i++)
    {
        close(pfds_vec[i].fd);
    }
    pfds_vec.clear();

    // Close all server listener sockets
    for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
    {
        if ((*it).getListenerFd() != -1)
        {
            close((*it).getListenerFd());
        }
    }

    fd_to_server.clear(); // Clear the map
}

WebService::~WebService()
{
    WebService::cleanup();
    DEBUG_MSG("Service status", "stopped");
}

// Return a listening socket
// getaddrinfo(NULL, PORT, &hints, &ai)
// NULL - hostname or IP address of the server, NULL means any available network, use for incoming connections
// PORT - port
// hints - criteria to resolve the ip address
// ai - pointer to a linked list of results returned by getaddrinfo().
// It holds the resolved network addresses that match the criteria specified in hints.
int WebService::get_listener_socket(const std::string &port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // Use my IP

    // Get address info
    if ((addrinfo_status = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0)
    {
        DEBUG_MSG("getaddrinfo error", gai_strerror(addrinfo_status));
        return -1;
    }

    int listener_fd;
    // Loop through results and bind to first valid one
    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener_fd < 0)
            continue;

        // Set socket options for reuse
        int yes = 1;
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            DEBUG_MSG("setsockopt error", strerror(errno));
            continue;
        }

        // Bind socket
        if (bind(listener_fd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener_fd);
            continue;
        }

        break;
    }

    freeaddrinfo(ai);

    // If we got here, it means we didn't get bound
    if (p == NULL)
    {
        DEBUG_MSG("Failed to bind", "");
        return -1;
    }

    // Listen
    if (listen(listener_fd, MAX_BACKLOG_UNACCEPTED_CON) == -1)
    {
        DEBUG_MSG("listen error", strerror(errno));
        return -1;
    }

    return listener_fd;
}

void WebService::addToPfdsVector(int new_fd)
{
    // Reserve space before adding to prevent reallocation during iteration
    if (pfds_vec.size() == pfds_vec.capacity())
    {
        pfds_vec.reserve(pfds_vec.capacity() * 2 + 1);
    }

    struct pollfd new_pollfd;
    new_pollfd.fd = new_fd;
    new_pollfd.events = POLLIN | POLLOUT;
    new_pollfd.revents = 0;

    pfds_vec.push_back(new_pollfd);
    DEBUG_MSG("Added new fd to pfds_vec", new_fd);
    DEBUG_MSG("Current pfds_vec size", pfds_vec.size());
}

void WebService::deleteFromPfdsVec(int &fd, size_t &i)
{
    if (pfds_vec[i].fd == fd)
    {
        pfds_vec.erase(pfds_vec.begin() + i);
        DEBUG_MSG("Erased fd from vector at index", i);
        DEBUG_MSG("Current pfds_vec size", pfds_vec.size());
    }
    else
    {
        DEBUG_MSG_1("Error: fd not found in vector at index", i);
    }
}

void WebService::deleteRequestObject(int &fd, Server &server)
{
    server.deleteRequestObject(fd);
}

void WebService::closeConnection(int &fd, size_t &i, Server &server)
{
    if (close(fd) == -1)
        DEBUG_MSG_1("Closing connection FD failed", strerror(errno));

    deleteFromPfdsVec(fd, i);
    deleteRequestObject(fd, server);
    DEBUG_MSG("Erased request object for fd", fd);
}

// creates a new httpRequest object for the new connection in the server object and maps the fd to the new httpRequest object
void WebService::createRequestObject(int new_fd, Server &server)
{
    HttpRequest newRequest;
    server.setRequestObject(new_fd, newRequest);
    DEBUG_MSG("New httpRequest object created for fd", new_fd);
}

void WebService::mapFdToServer(int new_fd, Server &server)
{
    this->fd_to_server[new_fd] = &server;
}

void WebService::newConnection(Server &server)
{
    DEBUG_MSG("New connection", "incoming");
    struct sockaddr_storage remoteaddr;
    addrlen = sizeof remoteaddr;
    int new_fd = accept(server.getListenerFd(), (struct sockaddr *)&remoteaddr, &addrlen);

    errno = 0;
    if (new_fd == -1)
    {
        DEBUG_MSG_1("Accept error", strerror(errno));
    }
    else
    {
        DEBUG_MSG("New connection accepted for server ", server.getName());
        DEBUG_MSG("On fd", new_fd);

        addToPfdsVector(new_fd);
        mapFdToServer(new_fd, server);
        createRequestObject(new_fd, server);
    }
}

// get a listening socket for each server
void WebService::setupSockets()
{
    for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
    {
        int listener_fd = get_listener_socket((*it).getPort());
        if (listener_fd == -1)
        {
            DEBUG_MSG_1("get_listener_socket failed", strerror(errno));

            throw std::runtime_error("Error getting listening socket for server: " + (*it).getName());
        }
        (*it).setListenerFd(listener_fd);
        addToPfdsVector(listener_fd);
        mapFdToServer(listener_fd, *it);
    }
    DEBUG_MSG("Total servers set up", servers.size());
}

int WebService::start()
{
    DEBUG_MSG("Server Status", "Starting");
    while (true)
    {
        // Store current size to prevent invalid access during iteration, to fix valgrind error

        // Use a reference to avoid copying
        std::vector<pollfd> &poll_fds = pfds_vec;
        int poll_count = poll(poll_fds.data(), pfds_vec.size(), POLL_TIMEOUT);

        if (poll_count == -1)
        {
            DEBUG_MSG_1("Poll error", strerror(errno));
            continue;
        }

        // Check CGI processes for timeouts
        CGI::checkRunningProcesses();

        // Iterate backwards to handle removals safely
        for (size_t i = pfds_vec.size(); i-- > 0;)
        {
            if (i >= poll_fds.size())
                continue; // Safety check

            pollfd &pollfd_obj = poll_fds[i];
            if (pollfd_obj.revents == 0)
                continue;

            // Check if fd exists in map of regular request or cgi requests before accessing
            if (fd_to_server.find(pollfd_obj.fd) == fd_to_server.end())
            {
                continue;
            }
            // if (WebService::cgi_fd_to_http_response.find(pollfd_obj.fd) == WebService::cgi_fd_to_http_response.end())
            // {
            //     continue;
            // }

            Server *server_obj = fd_to_server[pollfd_obj.fd];

            if (pollfd_obj.revents & POLLIN)
            {
                if (i < servers.size())
                    newConnection(*server_obj);
                else
                    receiveRequest(pollfd_obj.fd, i, *server_obj);
            }
            if (pollfd_obj.revents & POLLOUT)
            {
                sendResponse(pollfd_obj.fd, i, *server_obj);
            }
            if (pollfd_obj.revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                DEBUG_MSG("Closing connection due to error or hangup", pollfd_obj.fd);
                closeConnection(pollfd_obj.fd, i, *server_obj);
            }
        }

        // Check CGI processes
        // for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
        // {
        //     checkCGIProcesses(*it);
        // }
    }
}

// implement timeout for recv()?
void WebService::receiveRequest(int &fd, size_t &i, Server &server)
{
    // DEBUG_MSG("=== RECEIVE REQUEST ===", "");
    // DEBUG_MSG("Processing FD", fd);

    if (!server.getRequestObject(fd).complete)
    {
        errno = 0;
        int nbytes = recv(fd, buf, sizeof buf, 0);
        DEBUG_MSG("Bytes received", nbytes);

        if (nbytes == 0)
        {
            DEBUG_MSG("Receive closed succesfully", nbytes);
            closeConnection(fd, i, server);
        }
        else if (nbytes < 0)
        {
            DEBUG_MSG_1("Receive failed, error", strerror(errno));
            closeConnection(fd, i, server);
        }
        else
        {
            buf[nbytes] = '\0';
            server.getRequestObject(fd).raw_request.append(buf);
            DEBUG_MSG("Received data from fd", fd);
            DEBUG_MSG("Data received", buf);
            DEBUG_MSG("Request object raw_request", server.getRequestObject(fd).raw_request);

            // Try to parse headers if we haven't yet
            if (!server.getRequestObject(fd).headers_parsed &&
                server.getRequestObject(fd).raw_request.find("\r\n\r\n") != std::string::npos)
            {
                DEBUG_MSG("Request Status", "Parsing headers");
                try
                {
                    RequestParser::parseRawRequest(server.getRequestObject(fd));
                }
                catch (const std::exception &e)
                {
                    DEBUG_MSG("Error parsing request", e.what());
                    closeConnection(fd, i, server);
                    return;
                }
            }

            // If headers are parsed, try to parse body
            if (server.getRequestObject(fd).headers_parsed)
            {
                try
                {
                    RequestParser::parseRawRequest(server.getRequestObject(fd));
                }
                catch (const std::exception &e)
                {
                    DEBUG_MSG("Error parsing body", e.what());
                    closeConnection(fd, i, server);
                    return;
                }
            }

            DEBUG_MSG("Current body size", server.getRequestObject(fd).body.size());
            if (server.getRequestObject(fd).complete)
            {
                DEBUG_MSG("Request complete", "Ready to process");
            }
        }
    }
}

void WebService::sendResponse(int &fd, size_t &i, Server &server)
{
    HttpRequest request_obj = server.getRequestObject(fd);
    if (request_obj.complete)
    {
        HttpRequest request = server.getRequestObject(fd);
        HttpResponse response;
        ResponseHandler handler;
        handler.processRequest(fd, server, request, response);
        if (response.status_code != 0)
        {
            // Modify the pollfd to monitor POLLOUT for this FD
            pfds_vec[i].events = POLLOUT;
        }

        std::string responseStr = response.generateRawResponseStr();
        if (send(fd, responseStr.c_str(), responseStr.size(), 0) == -1)
        {
            DEBUG_MSG_1("Send error ", strerror(errno));
        }
        DEBUG_MSG("Response sent to fd", fd);

        if (response.close_connection == true)
        {
            closeConnection(fd, i, server);
        }
        else
        {
            server.resetRequestObject(fd);
        }
    }
}

void WebService::sigintHandler(int signal)
{
    if (signal == SIGINT)
    {
        DEBUG_MSG("Signal received", "SIGINT");
        DEBUG_MSG("Server status", "Shutting down");
        WebService::cleanup();
        exit(0);
    }
}

// void WebService::checkCGIProcesses(Server &server)
// {
//     for (std::map<int, HttpRequest>::iterator it = server.requests.begin();
//          it != server.requests.end(); ++it)
//     {
//         int client_fd = it->first;
//         HttpResponse &response = server.getResponseObject(client_fd);

//         if (response.is_cgi_running)
//         {
//             // Check if process has finished
//             int status;
//             pid_t result = waitpid(response.cgi_pid, &status, WNOHANG);

//             if (result > 0)
//             { // Process finished
//                 // Read remaining output
//                 char buffer[1024];
//                 std::string output;

//                 while (true)
//                 {
//                     ssize_t bytes = read(response.cgi_output_fd, buffer, sizeof(buffer) - 1);
//                     if (bytes <= 0)
//                         break;
//                     buffer[bytes] = '\0';
//                     output += buffer;
//                 }

//                 close(response.cgi_output_fd);
//                 response.is_cgi_running = false;

//                 // Process the CGI output and send response
//                 response.setBody(output);
//                 response.status_code = 200;
//             }
//             else if (result < 0)
//             { // Error occurred
//                 response.is_cgi_running = false;
//                 response.status_code = 500;
//             }
//             // result == 0 means process still running
//         }
//     }
// }