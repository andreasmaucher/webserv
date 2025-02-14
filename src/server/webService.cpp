/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webService.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/02/15 00:27:49 by mrizhakov        ###   ########.fr       */
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
    DEBUG_MSG_1("Added new fd to pfds_vec", new_fd);
    DEBUG_MSG_1("Current pfds_vec size", pfds_vec.size());
}

void WebService::deleteFromPfdsVec(int &fd, size_t &i)
{
    if (pfds_vec[i].fd == fd)
    {
        pfds_vec.erase(pfds_vec.begin() + i);
        DEBUG_MSG_2("WebService::deleteFromPfdsVec: Erased fd from vector at index", i);
        DEBUG_MSG_2("WebService::deleteFromPfdsVec: Current pfds_vec size", pfds_vec.size());
        // exit(1);
    }
    else
    {
        DEBUG_MSG_2("WebService::deleteFromPfdsVec: Error: fd not found in vector at index", i);
        // exit(1);
    }
}

void WebService::deleteFromPfdsVecForCGI(const int &fd)
{
    DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI need to delete the fd, ", fd);
    const int fd_to_delete = fd; // Local copy of the value
    DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI need to delete the fd, ", fd_to_delete);
    std::vector<pollfd>::iterator it;
    for (it = pfds_vec.begin(); it != pfds_vec.end(); ++it)
    {
        DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI looping throught the fds to delete, current fd ", it->fd);
        if (it->fd == fd_to_delete)
        {
            DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI will delete the fd, ", it->fd);

            WebService::pfds_vec.erase(it);
            DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI deleted the fd, ", fd_to_delete);
            // DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI deleted the it->fd, ", it->fd);

            break; // Exit after finding and removing
        }
    }

    // for (it = pfds_vec.begin(); it != pfds_vec.end(); it++)
    // {
    //     DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI CHECKING AGAIN looping throught the fds to delete, current fd ", it->fd);
    //     // if (it->fd == fd)
    //     // {
    //     //     DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI will delete the fd, ", it->fd);

    //     //     pfds_vec.erase(it);
    //     //     DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI deleted the fd, ", fd);

    //     //     break; // Exit after finding and removing
    //     // }
    // }
}

void WebService::deleteRequestObject(const int &fd, Server &server)
{
    server.deleteRequestObject(fd);
}

void WebService::closeConnection(const int &fd, size_t &i, Server &server)
{
    (void)i;
    const int fd_to_delete = fd;
    if (close(fd) == -1)
        DEBUG_MSG_2("Closing connection FD failed", fd);
    else
        DEBUG_MSG_2("Connection to FD closed succeeded", fd);

    // deleteFromPfdsVec(fd, i);
    deleteFromPfdsVecForCGI(fd_to_delete);

    DEBUG_MSG_2("WebService::closeConnection after deleteFromPfdsVecForCGI(fd); fd is ", fd);

    deleteRequestObject(fd_to_delete, server);
    DEBUG_MSG_2("Erased request object for fd", fd);
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

void WebService::printPollFds()
{
    DEBUG_MSG("=== POLL FDS STATUS ===", "");
    for (size_t i = 0; i < pfds_vec.size(); i++)
    {
        int fd = pfds_vec[i].fd;
        std::string fd_type;

        if (cgi_fd_to_http_response.find(fd) != cgi_fd_to_http_response.end())
        {
            fd_type = "CGI";
        }
        else if (fd_to_server.find(fd) != fd_to_server.end())
        {
            Server *server = fd_to_server[fd];
            if (fd == server->getListenerFd())
            {
                fd_type = "SERVER LISTENER";
            }
            else
            {
                fd_type = "SERVER CONNECTION";
            }
        }
        else
        {
            fd_type = "UNKNOWN";
        }

        DEBUG_MSG_2("FD: ", fd);
        DEBUG_MSG_2("Type: ", fd_type);
        DEBUG_MSG_2("Events: ", pfds_vec[i].events);
        DEBUG_MSG_2("Revents: ", pfds_vec[i].revents);
        DEBUG_MSG("-------------------", "");
    }
    DEBUG_MSG("=====================", "");
}

int WebService::start()
{
    DEBUG_MSG("Server Status", "Starting");
    bool skip_to_next_poll = false;
    size_t i = 0;
    (void)i;
    while (true)
    {
        skip_to_next_poll = false; // Flag to control outer loop skip

        int poll_count = poll(pfds_vec.data(), pfds_vec.size(), POLL_TIMEOUT);
        if (poll_count == -1)
        {
            DEBUG_MSG_1("Poll error", strerror(errno));
            continue;
        }
        printPollFds(); // Add this line here

        // Check CGI processes for timeouts

        DEBUG_MSG_2("-----------> Webservice::start() entering CGI check ", "");
        DEBUG_MSG_2("----------->  CGI::checkRunningProcesses(pfds_vec[i].fd ", pfds_vec[i].fd);
        DEBUG_MSG_2("----------->  CGI::checkRunningProcesses i is  ", i);

        // CGI::checkRunningProcesses(pfds_vec[i].fd);
        // Iterate backwards to handle removals safely
        DEBUG_MSG_2("-----------> Webservice::start() passed CGI check ", "");
        for (size_t i = pfds_vec.size(); i-- > 0;)
        {
            // sleep(1);
            DEBUG_MSG_2("-----------> Webservice::start() pfds_vec.size() ", pfds_vec.size());
            DEBUG_MSG_2("-----------> Webservice::start() pfds_vec[i].fd ", pfds_vec[i].fd);
            DEBUG_MSG_2("-----------> Webservice::start() i ", i);

            DEBUG_MSG_2("-----------> Webservice::start() entered pfds loop ", "");
            if (i >= pfds_vec.size())
            {
                DEBUG_MSG_2("-----------> Webservice::start() i >= pfds_vec.size() is true ", pfds_vec.size());

                continue;
            }
            if (pfds_vec[i].revents == 0)
            {
                DEBUG_MSG_2("-----------> Webservice::start() i >= pfds_vec[i].revents == 0 is true", "");
                continue;
            }
            if (cgi_fd_to_http_response.find(pfds_vec[i].fd) != cgi_fd_to_http_response.end())
            {
                DEBUG_MSG_2("Detected CGI FD, entering CGI::checkRunningProcesses(pfds_vec[i].fd);fd ", pfds_vec[i].fd);
                // sleep(1);

                CGI::checkRunningProcesses(pfds_vec[i].fd);

                // i--;
            }
            // else
            // {
            //     i--;
            //     break;
            // }

            if (fd_to_server.find(pfds_vec[i].fd) == fd_to_server.end())
            {

                // if (!(cgi_fd_to_http_response.find(pfds_vec[i].fd) == cgi_fd_to_http_response.end()))
                //     DEBUG_MSG_2("cgi_fd_to_http_response.find(pfds_vec[i].fd) , fd ", pfds_vec[i].fd);
                DEBUG_MSG_2("Detected NON-server FD, skipping loop, fd ", pfds_vec[i].fd);
                // usleep(100000);
                // continue;
                // i--;
                skip_to_next_poll = true; // Set flag to skip to next poll
                break;                    // Exit the for loop
            }
            // if (fd_to_server.find(pfds_vec[i].fd) != fd_to_server.end() || (cgi_fd_to_http_response.find(pfds_vec[i].fd) != cgi_fd_to_http_response.end()))
            // {
            //     if (fd_to_server.find(pfds_vec[i].fd) == fd_to_server.end())
            //         DEBUG_MSG_2("fd_to_server.find(pfds_vec[i].fd) found , fd ", pfds_vec[i].fd);
            //     if (!(cgi_fd_to_http_response.find(pfds_vec[i].fd) == cgi_fd_to_http_response.end()))
            //         DEBUG_MSG_2("cgi_fd_to_http_response.find(pfds_vec[i].fd) , fd ", pfds_vec[i].fd);
            //     DEBUG_MSG_2("Detected CGI process or non-server fd, skipping loop, fd ", pfds_vec[i].fd);
            //     usleep(100000);
            //     // continue;
            //     // skip_to_next_poll = true; // Set flag to skip to next poll
            //     // break;                    // Exit the for loop
            // }
            if (skip_to_next_poll)
            {
                // sleep(1);
                continue; // Skip to next iteration of while loop
            }
            DEBUG_MSG_2("Passed FD check --->  FD is either in fd_to_server nor in cgi_fd_to_http_response, fd ", pfds_vec[i].fd);
            // get server object from a particular connection fd
            Server *server_obj = fd_to_server[pfds_vec[i].fd];
            DEBUG_MSG_2("Passed Server assignment check --->, fd ", pfds_vec[i].fd);
            if (pfds_vec[i].revents & POLLIN)
            {
                if (i < servers.size())
                {
                    DEBUG_MSG_2("New connection ", pfds_vec[i].fd);
                    newConnection(*server_obj);
                }
                else
                {
                    DEBUG_MSG_2("Receive request  ", pfds_vec[i].fd);
                    receiveRequest(pfds_vec[i].fd, i, *server_obj);
                }
            }
            if (pfds_vec[i].revents & POLLOUT)
            {
                DEBUG_MSG_2("------->Send response  ", pfds_vec[i].fd);

                sendResponse(pfds_vec[i].fd, i, *server_obj);
            }
            if (pfds_vec[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                DEBUG_MSG_2("-------->Close connection  ", pfds_vec[i].fd);

                closeConnection(pfds_vec[i].fd, i, *server_obj);
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
            DEBUG_MSG_2("WebService::receiveRequest Receive closed succesfully", nbytes);
            closeConnection(fd, i, server);
        }
        else if (nbytes < 0)
        {
            DEBUG_MSG_2(" WebService::receiveRequest Receive failed, error", strerror(errno));
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
                    DEBUG_MSG_2("WebService::receiveRequest Error parsing request", e.what());
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
                    DEBUG_MSG_2("WebService::receiveRequest Error parsing body", e.what());
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
    DEBUG_MSG_2("------->WebService::sendResponse server.getRequestObject(fd) passed ", fd);

    if (request_obj.complete)
    {
        HttpRequest request = server.getRequestObject(fd);
        DEBUG_MSG_2("------->WebService::sendResponse server.getRequestObject(fd); passed ", fd);

        HttpResponse *response = new (HttpResponse);
        ResponseHandler handler;
        handler.processRequest(fd, server, request, *response);
        DEBUG_MSG_2("------->WebService::sendResponse handler.processRequest(fd, server, request, response); passed ", fd);

        // If request is a CGI, skip this part
        const Route *route = request.route; // Route is already stored in request
        DEBUG_MSG_2("------->WebService::sendResponse const Route *route = request.route; is the issue ", fd);

        if (!route->is_cgi)
        {
            if (response->status_code != 0 && response->status_code != 301)
            {
                // Modify the pollfd to monitor POLLOUT for this FD
                DEBUG_MSG_2("------->WebService::sendResponse pfds_vec[i].events = POLLOUT; is the issue ", fd);

                pfds_vec[i].events = POLLOUT;
                DEBUG_MSG_2("------->WebService::sendResponse pfds_vec[i].events = POLLOUT; passed ", fd);
            }

            std::string responseStr = response->generateRawResponseStr();
            DEBUG_MSG_2("------->WebService::sendResponse generateRawResponseStr(); passed ", fd);

            if (send(fd, responseStr.c_str(), responseStr.size(), 0) == -1)
            {
                DEBUG_MSG_2("Send error ", strerror(errno));
            }
            DEBUG_MSG_2("Response sent to fd", fd);

            DEBUG_MSG_2("WebService::sendResponse response.close_connection", response->close_connection);
            // Michaael: added obligatory close of connection for all cases to get rid of extra pfds
            response->close_connection = true;
            // sleep(10);

            if (response->close_connection == true)
            {
                DEBUG_MSG_2("WebService::sendResponse: Response sent to fd, closing connection", fd);

                closeConnection(fd, i, server);
            }
            else
            {
                server.resetRequestObject(fd);
            }
            delete response;
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

void WebService::setPollfdEventsToOut(int fd)
{
    for (size_t i = 0; i < pfds_vec.size(); ++i)
    {
        if (pfds_vec[i].fd == fd)
        {
            pfds_vec[i].events = POLLOUT;
            break; // Exit once found
        }
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