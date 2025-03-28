/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webService.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizakov <mrizakov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/03/28 15:01:53 by mrizakov         ###   ########.fr       */
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
        deleteFromPfdsVecForCGI(pfds_vec[i].fd);
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
    hints.ai_family = AF_INET; // IPv4
    // hints.ai_socktype = SOCK_STREAM | SOCK_NONBLOCK; // TCP
    hints.ai_socktype = SOCK_STREAM; // TCP

    hints.ai_flags = AI_PASSIVE; // Use my IP

    // Get address info
    if ((addrinfo_status = getaddrinfo(NULL, port.c_str(), &hints, &ai)) != 0)
    {
        DEBUG_MSG_2("getaddrinfo error", gai_strerror(addrinfo_status));
        return -1;
    }

    int listener_fd;
    // Loop through results and bind to first valid one
    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener_fd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK, p->ai_protocol);
        if (listener_fd < 0)
            continue;

        // Set socket options for reuse
        int yes = 1;
        if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            DEBUG_MSG_2("setsockopt error", strerror(errno));
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
        DEBUG_MSG_2("Failed to bind", "");
        return -1;
    }

    // Listen
    if (listen(listener_fd, MAX_BACKLOG_UNACCEPTED_CON) == -1)
    {
        DEBUG_MSG_2("listen error", strerror(errno));
        return -1;
    }

    return listener_fd;
}

int WebService::addToPfdsVector(int new_fd, bool isCGIOutput)
{
    // Reserve extra space if needed...
    if (pfds_vec.size() >= pfds_vec.capacity())
    {
        size_t new_capacity = pfds_vec.capacity() * 2 + 1;
        pfds_vec.reserve(new_capacity);
        DEBUG_MSG_1("Increased vector capacity to", new_capacity);
    }

    struct pollfd new_pollfd;
    new_pollfd.fd = new_fd;
    // If this is the CGI output pipe, listen for more events:
    if (isCGIOutput)
    {
        new_pollfd.events = POLLIN | POLLHUP | POLLERR;
    }
    else
    {
        new_pollfd.events = POLLIN | POLLOUT;
    }
    new_pollfd.revents = 0;

    pfds_vec.push_back(new_pollfd);
    DEBUG_MSG_1("Added new fd to pfds_vec", new_fd);
    DEBUG_MSG_1("Current pfds_vec size", pfds_vec.size());
    
    // Return the index of the newly added element
    return pfds_vec.size() - 1;
}

void WebService::deleteFromPfdsVec(int &fd, size_t &i)
{
    if (pfds_vec[i].fd == fd)
    {
        pfds_vec.erase(pfds_vec.begin() + i);
        DEBUG_MSG_2("WebService::deleteFromPfdsVec: Erased fd from vector at index", i);
        DEBUG_MSG_2("WebService::deleteFromPfdsVec: Current pfds_vec size", pfds_vec.size());
        //  exit(1);
    }
    else
    {
        DEBUG_MSG_2("WebService::deleteFromPfdsVec: Error: fd not found in vector at index", i);
        //  exit(1);
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
            //  DEBUG_MSG_2("WebService::deleteFromPfdsVecForCGI deleted the it->fd, ", it->fd);

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
    {
        DEBUG_MSG_2("Closing connection FD failed", fd);
    }
    else
    {
        DEBUG_MSG_2("Connection to FD closed succeeded", fd);
    }

    // deleteFromPfdsVec(fd, i);
    deleteFromPfdsVecForCGI(fd_to_delete);

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
    if (new_fd == -1)
    {
        DEBUG_MSG_1("Accept error", strerror(errno));
    }
    else
    {
        DEBUG_MSG("New connection accepted for server ", server.getName());
        DEBUG_MSG("On fd", new_fd);
        // TODO:change to pollin immediately

        addToPfdsVector(new_fd, false);
        setPollfdEventsToIn(new_fd);
        printPollFdStatus(findPollFd(new_fd));

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
            std::cerr << "Incorrect server configuration, please check the config file " << std::endl;

            throw std::runtime_error("Error getting listening socket for server: " + (*it).getName());
        }
        (*it).setListenerFd(listener_fd);
        addToPfdsVector(listener_fd, false);
        setPollfdEventsToIn(listener_fd);
        mapFdToServer(listener_fd, *it);
    }
    DEBUG_MSG("Total servers set up", servers.size());
}

std::string toString(int value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

// void CGI::checkAllCGIProcesses()
// {
//     time_t now = time(NULL);

//     // Iterate through all running CGI processes.
//     // (Assuming running_processes is a map keyed by PID or some unique ID.)
//     for (std::map<pid_t, CGI::CGIProcess>::iterator it = CGI::running_processes.begin();
//          it != CGI::running_processes.end();
//          /* no increment */)
//     {
//         pid_t pid = it->first;

//         CGI::CGIProcess &proc = it->second;
//         if ((now - proc.last_update_time) > CGI_TIMEOUT)
//         {
//             DEBUG_MSG_2("CGI timeout reached for pid", pid);

//             // Force kill the process, mark as finished and unsuccessful:
//             kill(pid, SIGKILL);
//             proc.process_finished = true;
//             proc.finished_success = false;

//             // Mark response as complete so that cleanup logic proceeds:
//             proc.response->complete = true;

//             // Send a timeout response:
//             proc.response->body = constructErrorResponse(504, "Gateway timeout");
//             proc.response->status_code = 504;
//             proc.response->reason_phrase = "Gateway timeout";
//             proc.response->close_connection = true;
//             sendCGIResponse(proc);

//             // Remove from any mappings and clean up FDs
//             WebService::cgi_fd_to_http_response.erase(proc.output_pipe);
//             WebService::deleteFromPfdsVecForCGI(proc.output_pipe);
//             WebService::deleteFromPfdsVecForCGI(proc.response_fd);
//             WebService::fd_to_server.erase(proc.response_fd);
//             proc.output_pipe = -1; // Mark as invalid

//             // Erase this CGI process from the running_processes map.
//             std::map<pid_t, CGI::CGIProcess>::iterator current = it;
//             ++it;
//             CGI::running_processes.erase(current);
//             delete proc.response;

//             continue;
//         }
//         ++it; // Only increment if no timeout occurred
//     }
// }

int WebService::start()
{
    DEBUG_MSG("Server Status", "Starting");
    // bool skip_to_next_poll = false;
    size_t i = 0;
    (void)i;
    // pfds_vec.reserve(1000);
    while (true)
    {
        // skip_to_next_poll = false; // Flag to control outer loop skip

        if (!CGI::running_processes.empty())
        {
            CGI::checkAllCGIProcesses();
            // printPollFds();
        }
        int poll_count = poll(pfds_vec.data(), pfds_vec.size(), POLL_TIMEOUT);
        if (poll_count == -1)
        {
            DEBUG_MSG_1("Poll error", strerror(errno));
            continue;
        }
        // sleep(1);
       // DEBUG_MSG_3("----------------------------------> Webservice::start() PASSED POLL ", "");
        // CGI::printRunningProcesses();

        // Check CGI processes for timeouts

        // CGI::checkRunningProcesses(pfds_vec[i].fd);
        // Iterate backwards to handle removals safely
        for (size_t i = pfds_vec.size(); i-- > 0;)
        {
            CGI::printRunningProcesses();

            // sleep(1);
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
            // if (!(pfds_vec[i].revents & (POLLIN | POLLHUP | POLLERR)))
            if (pfds_vec[i].revents == 0)
            {
                DEBUG_MSG_2("-----------> Webservice::start() i >= pfds_vec[i].revents == 0 is true", "");
                continue;
            }
            // printPollFdStatus(findPollFd(pfds_vec[i].fd));
            if (cgi_fd_to_http_response.find(pfds_vec[i].fd) != cgi_fd_to_http_response.end() &&
                (pfds_vec[i].revents & (POLLIN | POLLOUT | POLLHUP | POLLERR | POLLNVAL)))
            {
                DEBUG_MSG_2("Detected CGI FD, entering CGI::checkRunningProcesses(pfds_vec[i].fd);fd ", pfds_vec[i].fd);
                //  sleep(1);
                CGI::checkCGIProcess(pfds_vec[i].fd);
                // continue;

                // CGI::checkRunningProcesses(pfds_vec[i].fd);

                i--;
            }
            else
            {
                DEBUG_MSG_2("Not a CGI process ", pfds_vec[i].fd);
            }

            if (fd_to_server.find(pfds_vec[i].fd) == fd_to_server.end())
            {
                // if (!(cgi_fd_to_http_response.find(pfds_vec[i].fd) == cgi_fd_to_http_response.end()))
                //     DEBUG_MSG_2("cgi_fd_to_http_response.find(pfds_vec[i].fd) , fd ", pfds_vec[i].fd);
                DEBUG_MSG_2("Detected NON-server FD, skipping loop, fd ", pfds_vec[i].fd);
                // usleep(100000);
                continue;
                // i--;
                // skip_to_next_poll = true; // Set flag to skip to next poll
                // break;                    // Exit the for loop
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
            // if (skip_to_next_poll)
            // {
            //     // sleep(1);
            //     continue; // Skip to next iteration of while loop
            // }
            DEBUG_MSG_2("Passed FD check --->  FD is either in fd_to_server nor in cgi_fd_to_http_response, fd ", pfds_vec[i].fd);
            //  get server object from a particular connection fd
            Server *server_obj = fd_to_server[pfds_vec[i].fd];
            DEBUG_MSG_2("Passed Server assignment check --->, fd ", pfds_vec[i].fd);
            // sleep(1);
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
            else if (pfds_vec[i].revents & POLLOUT)
            {
                DEBUG_MSG_2("------->Send response  ", pfds_vec[i].fd);

                sendResponse(pfds_vec[i].fd, i, *server_obj);
            }
            else if (pfds_vec[i].revents & (POLLERR | POLLHUP | POLLNVAL))
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
/* 
// implement timeout for recv()?
void WebService::receiveRequest(int &fd, size_t &i, Server &server)
{
    DEBUG_MSG_3("=== RECEIVE REQUEST ===", "");
    DEBUG_MSG_3("Processing FD", fd);
    if (server.getRequestObject(fd).complete)
    {
        // Request is complete, so switch event monitoring to POLLOUT.
        setPollfdEventsToOut(fd);
        DEBUG_MSG_3("Switching FD to POLLOUT since request complete:", fd);
    }
    std::cout << "complete before receivece Request: " << server.getRequestObject(fd).complete << std::endl;
    if (!server.getRequestObject(fd).complete)
    {
        WebService::printPollFdStatus(WebService::findPollFd(fd));
        DEBUG_MSG_3("RECV started at receiveRequest", fd);
        std::cout << "RECV started at receiveRequest" << std::endl;
        int nbytes = recv(fd, buf, sizeof buf, 0);
        std::cout << "nbytes: " << nbytes << std::endl;
        DEBUG_MSG_3("RECV done at receiveRequest", fd);

        DEBUG_MSG_3("Bytes received", nbytes);
        DEBUG_MSG_3("Read :", buf);

        if (nbytes == 0)
        {
            DEBUG_MSG_2("WebService::receiveRequest Receive closed succesfully", nbytes);
            // Client closed connection - THIS IS IMPORTANT FOR BINARY FILES
            DEBUG_MSG_2("Client closed connection", fd);
            HttpRequest &request = server.getRequestObject(fd);
            request.client_closed_connection = true;
            
            // Don't close the connection yet - let the parser decide if we have enough data
            // Try to parse what we have
            try {
                RequestParser::parseRawRequest(server.getRequestObject(fd));
                
                // If we have a substantial amount of data and client closed connection,
                // this might be a complete binary upload
                if (server.getRequestObject(fd).body.size() > 1000) {
                    // Skip closing connection for now, process what we have
                    return;
                }
            } catch (const std::exception &e) {
                // Only close if parsing really fails
                closeConnection(fd, i, server);
            }
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
                    std::cout << "parseRawRequest in receiveRequest" << std::endl;
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

            DEBUG_MSG_3("Current body size", server.getRequestObject(fd).body.size());
            if (server.getRequestObject(fd).complete)
            {
                DEBUG_MSG_3("Request complete", "Ready to process");
                setPollfdEventsToOut(fd);
                DEBUG_MSG_3("Request is complete? ", server.getRequestObject(fd).complete);
            }
        }
    }
} */

//! ANDY new for binary files
void WebService::receiveRequest(int &fd, size_t &i, Server &server)
{
    HttpRequest &request = server.getRequestObject(fd);

    
    // Check if we're handling a binary upload 
    bool is_binary_upload = false;
    if (request.headers_parsed && 
        request.headers.find("Content-Type") != request.headers.end() && 
        request.headers["Content-Type"].find("multipart/form-data") != std::string::npos) {
        is_binary_upload = true;
    }
    
    if (request.complete)
    {
        // Request is complete, so switch event monitoring to POLLOUT.
        setPollfdEventsToOut(fd);
        return;
    }
    
    if (!request.complete)
    {
        WebService::printPollFdStatus(WebService::findPollFd(fd));
        DEBUG_MSG_3("RECV started at receiveRequest", fd);
        
        // For binary uploads, use a larger buffer for better performance
        char* recv_buffer = buf;
        size_t buffer_size = sizeof(buf);
        
        // Use a larger buffer for binary uploads
        char large_buffer[65536]; // 64KB buffer for binary data
        if (is_binary_upload) {
            recv_buffer = large_buffer;
            buffer_size = sizeof(large_buffer);
        }
        
        int nbytes = recv(fd, recv_buffer, buffer_size, 0);
        DEBUG_MSG_3("RECV done at receiveRequest", fd);
        DEBUG_MSG_3("Bytes received", nbytes);
        DEBUG_MSG_2("RECEIVE REQUEST", recv_buffer);

        
        if (nbytes <= 0)
        {
            if (nbytes == 0) {
                DEBUG_MSG_2("Client closed connection", fd);
                request.client_closed_connection = true;
                
                // For binary uploads, we need to be more careful
                if (is_binary_upload) {
                    // If we've received a substantial amount of data, try to process it
                    if (request.body.size() > 1000) {
                        try {
                            RequestParser::parseRawRequest(request);
                            // Don't close the connection yet - let the parser mark it complete
                            return;
                        } catch (const std::exception &e) {
                            // Only close if parsing really fails
                            closeConnection(fd, i, server);
                        }
                    } else {
                        closeConnection(fd, i, server);
                    }
                } else {
                    closeConnection(fd, i, server);
                }
            } else {
                DEBUG_MSG_2("Receive failed, error", strerror(errno));
                closeConnection(fd, i, server);
            }
        }
        else
        {
            // For binary uploads, we need to be careful with null bytes
            // Append using data() and size() instead of string methods
            


            
            
            request.raw_request.append(recv_buffer, nbytes);
            DEBUG_MSG_2("Checking request body size request.raw_request.size() ", request.raw_request.size());
            DEBUG_MSG_2("server.client_max_body_size ", server.client_max_body_size);
            if (request.raw_request.size() > server.client_max_body_size ||request.raw_request.size() > MAX_BODY_SIZE)
            {
                DEBUG_MSG_2("Request body size is greater than client_max_body_size", request.raw_request.size());
                request.complete = true;
                request.error_code = 413;
                setPollfdEventsToOut(fd);
                // response.status_code = 413;
                return;
            }
            
            DEBUG_MSG("Received data from fd", fd);
            
            // Try to parse headers if we haven't yet
            if (!request.headers_parsed &&
                request.raw_request.find("\r\n\r\n") != std::string::npos)
            {
                DEBUG_MSG("Request Status", "Parsing headers");
                try
                {
                    RequestParser::parseRawRequest(request);
                }
                catch (const std::exception &e)
                {
                    DEBUG_MSG_2("Error parsing request", e.what());
                    closeConnection(fd, i, server);
                    return;
                }
            }

            // If headers are parsed, try to parse body
            if (request.headers_parsed)
            {
                try
                {
                    RequestParser::parseRawRequest(request);
                }
                catch (const std::exception &e)
                {
                    DEBUG_MSG_2("Error parsing body", e.what());
                    closeConnection(fd, i, server);
                    return;
                }
            }

            DEBUG_MSG_3("Current body size", request.body.size());
            if (request.complete)
            {
                DEBUG_MSG_3("Request complete", "Ready to process");
                setPollfdEventsToOut(fd);
                DEBUG_MSG_3("Request is complete? ", request.complete);
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
        // DEBUG_MSG_2("------->WebService::sendResponse handler.processRequest(fd, server, request, response); passed ", fd);
        // Add null check before accessing route -> to catch faulty cgi requests (e.g. not .py)
        if (request.route == NULL)
        {
            // Handle invalid CGI or other requests without routes
            pfds_vec[i].events = POLLOUT;
            std::string responseStr = response->generateRawResponseStr();
            DEBUG_MSG_2("------->WebService::sendResponse sending responseStr ", responseStr.c_str());

            if (send(fd, responseStr.c_str(), responseStr.size(), 0) == -1)
            {
                DEBUG_MSG_2("Send error ", strerror(errno));
            }

            closeConnection(fd, i, server);
            delete response;
            return;
        }

        // If request is a CGI, skip this part
        const Route *route = request.route; // Route is already stored in request
        // DEBUG_MSG_2("------->WebService::sendResponse const Route *route = request.route; is the issue ", fd);

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
            //  Michaael: added obligatory close of connection for all cases to get rid of extra pfds
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
        if (WebService::pfds_vec[i].fd == fd)
        {
            DEBUG_MSG_3("SET FD TO POLLOUT", fd);

            WebService::pfds_vec[i].events = POLLOUT;
            // WebService::pfds_vec[i].revents = 0;
            break; // Exit once found
        }
    }
}

void WebService::setPollfdEventsToIn(int fd)
{
    for (size_t i = 0; i < pfds_vec.size(); ++i)
    {
        if (WebService::pfds_vec[i].fd == fd)
        {
            DEBUG_MSG_3("SET FD TO POLLIN", fd);

            WebService::pfds_vec[i].events = POLLIN;
            WebService::pfds_vec[i].revents = 0;
            break; // Exit once found
        }
    }
}
void WebService::setPollfdEvents(int fd, short events)
{
    for (size_t i = 0; i < pfds_vec.size(); ++i)
    {
        if (pfds_vec[i].fd == fd)
        {
            pfds_vec[i].events = events;
            break; // Exit once found
        }
    }
}

void WebService::printPollFds()
{
    DEBUG_MSG("=== POLL FDS STATUS ===", "");
    for (size_t i = 0; i < pfds_vec.size(); i++)
    {
        int fd = pfds_vec[i].fd;
        std::string fd_type1;
        std::string fd_type2;
        std::string fd_type3;
        std::string fd_type4;

        std::string connection_type;
        if (cgi_fd_to_http_response.find(fd) != cgi_fd_to_http_response.end())
        {
            fd_type1 = " CGI cgi_fd_to_http_response ";
        }
        if (fd_to_server.find(fd) != fd_to_server.end())
        {
            Server *server = fd_to_server[fd];
            if (fd == server->getListenerFd())
            {
                fd_type2 = " SERVER LISTENER ";
            }
            else
            {
                fd_type2 = "SERVER CONNECTION fd_to_server";
                // Check if this is request or response fd
            }
        }

        // fd_type3 = "UNKNOWN";

        // if (running_processes.find)

        DEBUG_MSG_3("FD: ", fd);
        DEBUG_MSG_3("Type: ", fd_type1 + connection_type);
        DEBUG_MSG_3("Type: ", fd_type2 + connection_type);
        DEBUG_MSG_3("Type: ", fd_type3 + connection_type);
        DEBUG_MSG_3("Type: ", fd_type3 + connection_type);

        // Print events with their values and meanings
        std::string event_str = "";
        if (pfds_vec[i].events & POLLIN)
            event_str += "POLLIN(1) ";
        if (pfds_vec[i].events & POLLPRI)
            event_str += "POLLPRI(4) ";
        if (pfds_vec[i].events & POLLOUT)
            event_str += "POLLOUT(4) ";
        if (pfds_vec[i].events & POLLERR)
            event_str += "POLLERR(8) ";
        if (pfds_vec[i].events & POLLHUP)
            event_str += "POLLHUP(16) ";
        if (pfds_vec[i].events & POLLNVAL)
            event_str += "POLLNVAL(32) ";
        DEBUG_MSG_3("Events(" + toString(pfds_vec[i].events) + "): ", event_str);
        // Print revents with their values and meanings
        std::string revent_str = "";
        if (pfds_vec[i].revents & POLLIN)
            revent_str += "POLLIN(1) ";
        if (pfds_vec[i].revents & POLLPRI)
            revent_str += "POLLPRI(4) ";
        if (pfds_vec[i].revents & POLLOUT)
            revent_str += "POLLOUT(4) ";
        if (pfds_vec[i].revents & POLLERR)
            revent_str += "POLLERR(8) ";
        if (pfds_vec[i].revents & POLLHUP)
            revent_str += "POLLHUP(16) ";
        if (pfds_vec[i].revents & POLLNVAL)
            revent_str += "POLLNVAL(32) ";
        DEBUG_MSG_3("Revents(" + toString(pfds_vec[i].revents) + "): ", revent_str);
        DEBUG_MSG_3("-------------------", "");
    }
    DEBUG_MSG_3("=====================", "");
}

struct pollfd *WebService::findPollFd(int fd)
{
    for (size_t i = 0; i < pfds_vec.size(); ++i)
    {
        if (pfds_vec[i].fd == fd)
        {
            return &pfds_vec[i];
        }
    }
    return NULL; // Return NULL if fd not found
}

std::string WebService::checkPollfdEvents(int fd)
{
    std::string return_str = "";
    for (size_t i = 0; i < pfds_vec.size(); ++i)
    {
        if (pfds_vec[i].fd == fd)
        {
            // DEBUG_MSG_2("Checking events for FD:", fd);

            // Check events
            std::string event_str = "";
            if (pfds_vec[i].events & POLLIN)
                event_str += "POLLIN ";
            if (pfds_vec[i].events & POLLOUT)
                event_str += "POLLOUT ";
            return_str += "Events:" + event_str;

            // Check revents
            std::string revent_str = "";
            if (pfds_vec[i].revents & POLLIN)
                revent_str += "POLLIN ";
            if (pfds_vec[i].revents & POLLOUT)
                revent_str += "POLLOUT ";
            if (pfds_vec[i].revents & POLLERR)
                revent_str += "POLLERR ";
            if (pfds_vec[i].revents & POLLHUP)
                revent_str += "POLLHUP ";
            if (pfds_vec[i].revents & POLLNVAL)
                revent_str += "POLLNVAL ";
            if (pfds_vec[i].revents & POLLPRI)
                revent_str += "POLLPRI ";

            return_str += " Revents:" + revent_str;
            return return_str;
        }
    }
    return_str = "FD not found in pfds_vec:";
    return return_str;
}

void WebService::printPollFdStatus(pollfd *pollfd)
{

    int fd = pollfd->fd;
    std::string fd_type1 = "";
    std::string fd_type2 = "";
    std::string fd_type3 = "";
    std::string fd_type4 = "";
    std::string fd_type5 = "";
    std::string fd_type3_revents = "";
    std::string fd_type4_revents = "";

    std::string connection_type;
    if (WebService::cgi_fd_to_http_response.find(fd) != WebService::cgi_fd_to_http_response.end())
    {
        fd_type1 = " Yes " + toString(fd);
    }
    if (WebService::fd_to_server.find(fd) != WebService::fd_to_server.end())
    {
        Server *server = WebService::fd_to_server[fd];
        if (fd == server->getListenerFd())
        {
            fd_type2 = " Yes, SERVER LISTENER ";
        }
        else
        {
            fd_type2 = " Yes, client connection " + toString(fd);
            // Check if this is request or response fd
        }
    }
    for (std::map<pid_t, CGI::CGIProcess>::iterator it = CGI::running_processes.begin();
         it != CGI::running_processes.end(); ++it)
    {
        pid_t pid = it->first;
        CGI::CGIProcess &proc = it->second;
        // Check if fd matches either the output pipe or response fd
        if (proc.output_pipe == fd || proc.response_fd == fd)
        {
            if (proc.output_pipe == fd)
            {
                fd_type3 = "CGI output_pipe : " + toString(fd);
                fd_type3_revents = WebService::checkPollfdEvents(proc.output_pipe);
            }
            else
                fd_type3 = " No";
            if (proc.response_fd == fd)
            {
                fd_type4 = "CGI response_fd : " + toString(fd);
                fd_type4_revents = WebService::checkPollfdEvents(proc.response_fd);
            }
            else
                fd_type4 = " No";
            fd_type5 = "PID : " + toString(pid);
        }
    }
    DEBUG_MSG_3("FD: ", fd);
    DEBUG_MSG_3("Type - cgi_fd_to_http_response ", fd_type1);
    DEBUG_MSG_3("Type - fd_to_server ", fd_type2);
    DEBUG_MSG_3("Type - CGI::running_processes output_pipe", fd_type3);
    DEBUG_MSG_3("", fd_type3_revents);
    DEBUG_MSG_3("Type - CGI::running_processes response_fd", fd_type4);
    DEBUG_MSG_3("", fd_type4_revents);
    DEBUG_MSG_3("Type - CGI::running_processes ", fd_type5);

    DEBUG_MSG_3("-------------------", "");
}
