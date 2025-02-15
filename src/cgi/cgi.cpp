#include "../../include/cgi.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/httpResponse.hpp"
#include "../../include/webService.hpp"
#include "../../include/responseHandler.hpp"

#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <ctime>

// Default constructor for the CGI class
CGI::CGI() : clientSocket(-1), scriptPath(""), method(""), queryString(""), requestBody("") {}

// Static method to check if a given path is a CGI request
// Returns true if the path contains "/cgi-bin/", ".py", or ".cgi"
bool CGI::isCGIRequest(const std::string &path)
{
    size_t start = path.find("/cgi-bin/") + 9;  // +9 to skip "/cgi-bin/"
    size_t end = path.find('/', start);         // Find next '/' or end of string
    if (end == std::string::npos) {
        end = path.length();
    }
    std::string scriptName = path.substr(start, end - start);
    return (scriptName.length() > 3 && scriptName.substr(scriptName.length() - 3) == ".py");
}

/*
1. gets current working directory
2. removes leading "/cgi-bin" from uri e.g. "/cgi-bin/script.py" -> "script.py"
3. combines with project root
4. returns full path
*/
std::string CGI::resolveCGIPath(const std::string &uri)
{
    char buffer[PATH_MAX];
    if (getcwd(buffer, PATH_MAX) == NULL)
    {
        throw std::runtime_error("Failed to get current working directory");
    }
    std::string projectRoot = std::string(buffer);

    // Find where the script name ends (at .py)
    size_t scriptEnd = uri.find(".py");
    if (scriptEnd == std::string::npos)
    {
        throw std::runtime_error("No .py script found in URI");
    }
    scriptEnd += 3; // include the ".py"

    // Extract just the script part (without PATH_INFO)
    std::string scriptUri = uri.substr(0, scriptEnd);

    // Remove "/cgi-bin" from the script path
    std::string relativePath = scriptUri.substr(8);

    std::string fullPath = projectRoot + "/cgi-bin" + relativePath;

    DEBUG_MSG("Project root", projectRoot);
    DEBUG_MSG("URI", uri);
    DEBUG_MSG("Script URI", scriptUri);
    DEBUG_MSG("Full resolved path", fullPath);

    // Check if script exists
    if (access(fullPath.c_str(), F_OK) == -1)
    {
        throw std::runtime_error("Script not found: " + fullPath);
    }

    return fullPath;
}
/* std::string CGI::resolveCGIPath(const std::string &uri)
{
    char buffer[PATH_MAX];
    if (getcwd(buffer, PATH_MAX) == NULL)
    {
        throw std::runtime_error("Failed to get current working directory");
    }
    std::string projectRoot = std::string(buffer);
    DEBUG_MSG("Project root", projectRoot);
    DEBUG_MSG("URI", uri);

    // Make sure cgi-bin exists in project root
    std::string relativePath = uri.substr(8); // removes "/cgi-bin"
    std::string fullPath = projectRoot + "/cgi-bin" + relativePath;
    DEBUG_MSG("Full resolved path", fullPath);
    return fullPath;
} */

// helper function to construct error response for non-existent or non-executable scripts
std::string CGI::constructErrorResponse(int status_code, const std::string &message)
{
    CGI cgi; // Create temporary object
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << cgi.getStatusMessage(status_code) << "\r\n";
    response << "Content-Type: text/plain\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << message;
    return response.str();
}

// Helper function to get status message
std::string CGI::getStatusMessage(int status_code)
{
    switch (status_code)
    {
    case 200:
        return "OK";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 500:
        return "Internal Server Error";
    default:
        return "Unknown";
    }
}

// Helper function to extract path info
std::string CGI::extractPathInfo(const std::string &uri)
{
    size_t scriptEnd = uri.find(".py");
    if (scriptEnd == std::string::npos)
    {
        return "";
    }
    scriptEnd += 3; // move past ".py"

    if (scriptEnd >= uri.length())
    {
        return "";
    }

    std::string pathInfo = uri.substr(scriptEnd + 1); // +1 to skip the leading '/'

    // Additional security checks
    if (pathInfo.find(".py") != std::string::npos)
    {
        throw std::runtime_error("Invalid PATH_INFO: Cannot contain .py files");
    }

    // Optional: Add more restrictions on allowed file types
    const std::string allowed_extensions[] = {".jpg", ".jpeg", ".png", ".gif"};
    bool is_allowed = false;
    for (size_t i = 0; i < sizeof(allowed_extensions) / sizeof(allowed_extensions[0]); ++i)
    {
        if (pathInfo.length() >= allowed_extensions[i].length() &&
            pathInfo.substr(pathInfo.length() - allowed_extensions[i].length()) == allowed_extensions[i])
        {
            is_allowed = true;
            break;
        }
    }

    if (!is_allowed)
    {
        throw std::runtime_error("Invalid file type in PATH_INFO");
    }

    return pathInfo;
}

/*
1. converts URI to full path (e.g., "/your/server/path/cgi-bin/script.py")
2. check permissions: F_OK: Checks if file exists, X_OK: Checks if file is executable
3. set environment
4. execute cgi (creates pipe, forks a process and executes the script)
5. response: store script output in request body and mark request as complete
*/
void CGI::handleCGIRequest(int &fd, HttpRequest &request, HttpResponse &response)
{
    DEBUG_MSG("Starting CGI Request", request.uri);
    DEBUG_MSG_2("CGI::handleCGIRequest receiving FD is  ", fd);
    executeCGI(fd, response, request);
/*     catch (const std::exception &e)
    {
        std::cout << "start handleCGIRequest 5" << std::endl;
        DEBUG_MSG_1("CGI Error", e.what());
        response.status_code = 500;
        response.body = "Internal Server Error: " + std::string(e.what());
        response.close_connection = true;
        response.complete = true;
    } */
}

/*
Sets up the environment variables for the CGI script; second part converts http headers to env variables
*/
char **CGI::setCGIEnvironment(const HttpRequest &httpRequest) const
{
    std::vector<std::string> env_strings;
    env_strings.push_back("REQUEST_METHOD=" + httpRequest.method);    // GET, POST, DELETE
    env_strings.push_back("QUERY_STRING=" + httpRequest.queryString); // everything after ? in URL
    env_strings.push_back("SCRIPT_NAME=" + httpRequest.uri);          // path to the CGI script
    env_strings.push_back("SERVER_PROTOCOL=" + httpRequest.version);  // Usually HTTP/1.1
    // Convert content length to string using stringstream (C++98 compliant)
    std::stringstream ss;
    ss << httpRequest.body.length();
    env_strings.push_back("CONTENT_LENGTH=" + ss.str()); // length of POST data
    // Add Content-Type if present (crucial for multipart form data like file uploads)
    std::map<std::string, std::string>::const_iterator it = httpRequest.headers.find("Content-Type");
    if (it != httpRequest.headers.end())
    {
        env_strings.push_back("CONTENT_TYPE=" + it->second);
        DEBUG_MSG("CGI Content-Type", it->second);
    }
    // Convert strings to char* array
    char **env_array = new char *[env_strings.size() + 1];
    for (size_t i = 0; i < env_strings.size(); i++)
    {
        env_array[i] = strdup(env_strings[i].c_str());
    }
    env_array[env_strings.size()] = NULL;
    return env_array;
}

void CGI::postRequest(int pipe_in[2])
{
    if (method == "POST")
    {
        if (!requestBody.empty())
        {
            std::cerr << "CGI POST: Starting to write POST data" << std::endl;
            std::cerr << "POST data length: " << requestBody.length() << std::endl;

            const size_t CHUNK_SIZE = 4096;
            size_t total_written = 0;
            const char *data = requestBody.c_str();
            size_t remaining = requestBody.length();

            while (remaining > 0)
            {
                size_t to_write = std::min(CHUNK_SIZE, remaining);
                ssize_t written = write(pipe_in[1], data + total_written, to_write);

                if (written == -1)
                {
                    std::cerr << "Write error: " << strerror(errno) << std::endl;
                    close(pipe_in[1]);
                    throw std::runtime_error("Failed to write to CGI input pipe");
                }

                total_written += written;
                remaining -= written;

                std::cerr << "Written " << written << " bytes, total " << total_written
                          << " of " << requestBody.length() << std::endl;
            }

            std::cerr << "CGI POST: Finished writing POST data" << std::endl;
        }
        close(pipe_in[1]); // Close write end after writing
    }
    else
    {
        close(pipe_in[1]); // Close write end immediately for non-POST requests
    }
}

pid_t CGI::runChildCGI(int pipe_in[2], int pipe_out[2], HttpRequest &request)
{
    if (pipe(pipe_in) == -1)
    {
        DEBUG_MSG("Pipe creation for stdin failed", strerror(errno));
        throw std::runtime_error("Pipe creation for stdin failed");
    }
    DEBUG_MSG("Input pipe - Read FD", pipe_in[0]);
    DEBUG_MSG("Input pipe - Write FD", pipe_in[1]);

    // Create output pipe
    if (pipe(pipe_out) == -1)
    {
        DEBUG_MSG("Pipe creation for stdout failed", strerror(errno));
        // Close previously opened pipe_in before throwing
        close(pipe_in[0]);
        close(pipe_in[1]);
        throw std::runtime_error("Pipe creation for stdout failed");
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        DEBUG_MSG("Fork failed", strerror(errno));
        // Close all opened file descriptors before throwing
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        throw std::runtime_error("Fork failed");
    }

    if (pid == 0)
    { // Child process
        DEBUG_MSG("Child process", "started");

        // Redirect stdin to pipe_in[0]
        if (dup2(pipe_in[0], STDIN_FILENO) == -1)
        {
            DEBUG_MSG("Child: dup2 for stdin failed", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to pipe_out[1]
        if (dup2(pipe_out[1], STDOUT_FILENO) == -1)
        {
            DEBUG_MSG("Child: dup2 for stdout failed", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Close unused file descriptors in child
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);

        // Set up environment variables
        char **env_array = setCGIEnvironment(request);
        // Execute the CGI script
        const char *pythonPath = PYTHON_PATH;
        char *const args[] = {
            const_cast<char *>(pythonPath),
            const_cast<char *>(scriptPath.c_str()),
            NULL};
        execve(pythonPath, args, env_array);
        // Clean up env_array if execve fails
        for (char **env = env_array; *env != NULL; env++)
        {
            free(*env);
        }
        delete[] env_array;
        exit(EXIT_FAILURE);
    }
    return pid;
}

/*
CGI LOGIC:
1. Creates communication channels (pipes) between server and CGI script
2. Forks into two processes (parent and child)
3. Child process executes the CGI script
4. Parent process reads the script's output
5. Returns the output as a string
*/
void CGI::executeCGI(int &fd, HttpResponse &response, HttpRequest &request)
{
    // Define pipes for stdin and stdout
    int status = 0;
    (void)status;
    int pipe_in[2];  // Parent writes to pipe_in[1], child reads from pipe_in[0]
    int pipe_out[2]; // Child writes to pipe_out[1], parent reads from pipe_out[0]
    DEBUG_MSG("Output pipe - Read FD", pipe_out[0]);
    DEBUG_MSG("Output pipe - Write FD", pipe_out[1]);
    pid_t pid = runChildCGI(pipe_in, pipe_out, request);
    DEBUG_MSG_2("---------------->CGI: executeCGI: new CGI process added: PID ", pid);

    // Parent process
    // Add process to tracking map right after fork
    addProcess(pid, pipe_out[0], fd, &request, &response);

    // Close unused file descriptors in parent
    close(pipe_in[0]);  // Close read end of input pipe
    close(pipe_out[1]); // Close write end of output pipe

    // If POST request, write the request body to the CGI's stdin
    postRequest(pipe_in);

    pollfd pollfd_obj;
    pollfd_obj.fd = pipe_out[0];
    pollfd_obj.revents = POLLIN;
    WebService::addToPfdsVector(pollfd_obj.fd);
    DEBUG_MSG_2("CGI: WebService::addToPfdsVector added fd: ", pollfd_obj.fd);
    WebService::cgi_fd_to_http_response[pollfd_obj.fd] = &response;
    DEBUG_MSG_1("Waitpid will start, pid : ", pid);
    waitpid(-1, &status, WNOHANG);
}

std::map<pid_t, CGI::CGIProcess> CGI::running_processes;

void CGI::addProcess(pid_t pid, int output_pipe, int response_fd, HttpRequest *req, HttpResponse *response)
{
    CGIProcess proc;
    proc.start_time = time(NULL);
    proc.output_pipe = output_pipe;
    proc.request = req;
    proc.response_fd = response_fd;
    proc.response = response;

    DEBUG_MSG_1("Adding new CGI process PID", pid);
    running_processes[pid] = proc;
}

void CGI::cleanupProcess(pid_t pid)
{
    if (running_processes.find(pid) != running_processes.end())
    {
        close(running_processes[pid].output_pipe);
        running_processes.erase(pid);
        DEBUG_MSG_2("------>Cleaned up CGI process", pid);
    }
    else
        DEBUG_MSG_2("------->Could not find CGI process to cleanup ", pid);
}

void CGI::sendCGIResponse(CGIProcess &proc)
{

    if (proc.response->complete == true)
    {
        if (isfdOpen(proc.response_fd))
        {
            DEBUG_MSG_2("SG1 ", "");
            proc.response->setHeader("Content-Type", "text/plain");
            proc.response->version = "HTTP/1.1";
            std::ostringstream oss;
            oss << proc.response->body.length();
            proc.response->setHeader("Content-Length", oss.str());

            proc.response->close_connection = true;

            WebService::setPollfdEventsToOut(proc.response_fd);
            DEBUG_MSG_2("SG2 ", "");
            ResponseHandler::responseBuilder(*proc.response);
            DEBUG_MSG_2("SG3 ", "");
            std::string responseStr = proc.response->generateRawResponseStr();
            DEBUG_MSG_2("---------->responseStr ", responseStr);
            ssize_t bytes_sent = send(proc.response_fd, responseStr.c_str(), responseStr.size(), 0);
            if (bytes_sent == -1)
            {
                DEBUG_MSG("Send error", strerror(errno));
            }
            else
            {
                DEBUG_MSG_2("Bytes sent:", bytes_sent);
                if (bytes_sent < (ssize_t)responseStr.size())
                {
                    DEBUG_MSG("Warning: Not all data was sent", "");
                }
            }
            if (close(proc.response_fd) == -1)
                DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Closing connection FD failed", strerror(errno));
            DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Closing sending pipe ", proc.response_fd);
        }
        else
        {
            DEBUG_MSG_2("FD is closed or invalid", proc.response_fd);
        }
    }
}

void CGI::readFromCGI(pid_t pid, CGIProcess &proc)
{
    ssize_t bytes_read;
    char buffer[MAX_CGI_BODY_SIZE];
    (void)pid;

    if ((bytes_read = read(proc.output_pipe, buffer, sizeof(buffer))) > 0)
    {
        proc.response->body += buffer;
        // proc.response->body.append(buffer, bytes_read);
        DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child finished, reading from pipe, pid ", pid);
        DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child finished, reading from pipe, bytes_read ", bytes_read);

        if (WIFEXITED(proc.status) && WEXITSTATUS(proc.status) == 0)
        {
            DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child finished, reading from pipe, (WIFEXITED(proc.status) && WEXITSTATUS(proc.status) == 0) ", strerror(errno));

            // proc.process_finished = true;
            // proc.finished_success = true;
            // proc.response->complete = true;
        }

        // Added Martin's suggestion;
        // proc.response->complete = true;
        return;
    }
    if (bytes_read == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() errno == EAGAIN || errno == EWOULDBLOCK", errno);

            return;
        }
        if (errno == ECHILD)
        {
            proc.process_finished = true;
            proc.finished_success = true;
            proc.response->complete = true;
        }
        else
        {
            DEBUG_MSG_2("Read error: ", strerror(errno));
            proc.response->complete = true;
            proc.response->status_code = 500;
            proc.finished_success = false;

            proc.process_finished = true;
        }
        if (close(proc.output_pipe) != 0)
            DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() proc.response_fd pipe could not be closed  ", proc.output_pipe);
        else
            DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() proc.response_fd closed  ", proc.output_pipe);
    }
    if (bytes_read == 0)
    {
        proc.process_finished = true;

        proc.finished_success = true;
        proc.response->complete = true;
        proc.response->status_code = 200;
        proc.response->version = "HTTP/1.1";
        proc.response->reason_phrase = "OK";
        if (!proc.finished_success)
        {
            proc.response->status_code = 500; // Internal Server Error
        }
        DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child finished, End of file reached", "");
        if (close(proc.output_pipe) != 0)
            DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() proc.response_fd pipe could not be closed  ", proc.output_pipe);
        else
            DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() proc.response_fd closed  ", proc.output_pipe);
    }
}

void CGI::checkRunningProcesses()
{
    if (running_processes.empty())
        return;
    pid_t return_pid;
    std::map<pid_t, CGIProcess>::iterator it = running_processes.begin();
    while (it != running_processes.end())
    {
        // sleep(1);
        pid_t pid = it->first;
        CGIProcess &proc = it->second;
        // char buffer[MAX_CGI_BODY_SIZE];
        // TODO: Michael - check why buffer is not being used, add exit code statuses, add check for max body size
        DEBUG_MSG_2("Waitpid will start, pid : ", pid);
        DEBUG_MSG_2("Waitpid will start, current process pid : ", pid);
        return_pid = waitpid(pid, &proc.status, WNOHANG);
        if (!proc.process_finished)
        {
            if (return_pid == -1)
            {
                if (errno == ECHILD)
                {
                    // Real error, not just "no child process"
                    DEBUG_MSG_2("Waitpid error: ", strerror(errno));
                    readFromCGI(pid, proc);
                    // proc.process_finished = true;
                    // proc.finished_success = false;
                    // TODO: set errror codes and appopriate error response
                }
                else
                {
                    proc.process_finished = true;
                    proc.finished_success = false;
                    proc.response->complete = true;
                    // TODO set error response
                }
                DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child waiting error, pid ", pid);
                DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child waiting error is ", strerror(errno));
            }
            else if (return_pid == pid)
            {
                if (WIFEXITED(proc.status) && WEXITSTATUS(proc.status) == 0)
                {
                    DEBUG_MSG_2("Process finished successfully: ", pid);
                    DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() return_pid == pid, WIFEXITED(proc.status) && WEXITSTATUS(proc.status) == 0 ", pid);
                    readFromCGI(pid, proc);
                    // sleep(1);
                }
                else
                {
                    DEBUG_MSG_2("Process finished with error: ", strerror(errno));
                    std::cout << "WEXITSTATUS(proc.status): " << WEXITSTATUS(proc.status) << std::endl;
                    std::cout << "WIFEXITED(proc.status): " << WIFEXITED(proc.status) << std::endl;
                    std::cout << "proc.response_body " << proc.response->body << std::endl;
                    proc.process_finished = true;
                    proc.finished_success = false;
                    // TODO: set errror codes and appopriate error response
                    DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() FINISHED CHILD done read with error ", pid);
                }
                DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() FINISHED CHILD done  ", pid);
            }
            else if (return_pid == 0)
            {
                time_t current_time = time(NULL);
                DEBUG_MSG_2("-------------->Webservice::CGI::checkRunningProcesses() CHILD STILL RUNNING, pid ", pid);
                DEBUG_MSG_2("CGI process timed out, killing PID ", pid);
                DEBUG_MSG_2("Current time ", current_time);
                DEBUG_MSG_2("proc.start_time ", proc.start_time);
                DEBUG_MSG_2("CGI_TIMEOUT", CGI_TIMEOUT);
                DEBUG_MSG_2("current_time - proc.start_time ", current_time - proc.start_time);
                if (current_time - proc.start_time > CGI_TIMEOUT)
                {
                    DEBUG_MSG_2("CGI timeout triggered ", current_time - proc.start_time);
                    DEBUG_MSG_2("Process still running after SIGTERM, sending SIGKILL", pid);
                    kill(pid, SIGKILL); // Force kill if still running
                    proc.process_finished = true;
                    proc.finished_success = false;
                    DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child killed, will send a 504, CGItimeout response here, pid ", pid);
                    DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Sending FD is  ", proc.response_fd);
                }
            }
        }
        DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Time to send a response and cleanup pid ", pid);
        DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() proc.process_finished ", proc.process_finished);
        DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() proc.finished_success ", proc.finished_success);
        DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() proc.response->complete ", proc.response->complete);
        if (proc.finished_success && proc.process_finished)
        {
            DEBUG_MSG_2("---------->Webservice::CGI::checkRunningProcesses() sendCGIResponse(pid, proc);  ", pid);
            proc.response->complete = true; // cleanup
            DEBUG_MSG_2("==================>Webservice::CGI::checkRunningProcesses() SEND RESPONSE on fd  ", proc.response_fd);
            proc.response->status_code = 200;
            proc.response->reason_phrase = "OK";
            proc.response->version = "HTTP/1.1";

            proc.response->close_connection = true;
            sendCGIResponse(proc);
            WebService::deleteFromPfdsVecForCGI(proc.response_fd);
            delete proc.response;
            // sleep(1);
        }
        if (!proc.finished_success && proc.process_finished)
        {
            DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() SEND ERROR RESPONSE  ", pid);
            proc.response->complete = true; // cleanup
            std::cout << "timeout here" << std::endl;
            proc.response->body = constructErrorResponse(504, "Gateway timeout");
            proc.response->status_code = 504;
            proc.response->reason_phrase = "Gateway timeout";
            proc.response->close_connection = true;
            sendCGIResponse(proc);
            WebService::deleteFromPfdsVecForCGI(proc.response_fd);
            delete proc.response;
        }
        if (proc.process_finished == true && proc.response->complete == true)
        {
            std::map<pid_t, CGIProcess>::iterator next_it = it;
            ++next_it;

            DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() WebService::deleteFromPfdsVecForCGI(proc.output_pipe) passed  ", pid);
            std::vector<pollfd>::iterator it2;
            for (it2 = WebService::pfds_vec.begin(); it2 != WebService::pfds_vec.end(); ++it2)
            {
                DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() proc.response_fd is  ", proc.output_pipe);
                if (it2->fd == proc.output_pipe)
                {
                    if (WebService::cgi_fd_to_http_response.erase(it2->fd) == 1)
                        DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() deleted cgi_fd_to_http_respone ", it2->fd);
                    DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() running_processes[pid].output_pipe after erase; ", running_processes[pid].output_pipe);
                    DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses()  pfds_vec.fd to delete ", it2->fd);
                    DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses()  pfds_vec.fd after deletion ", it2->fd);
                    break; // Exit after finding and removing
                }
            }
            WebService::deleteFromPfdsVecForCGI(proc.output_pipe);
            running_processes.erase(pid);
            it = next_it;
            // sleep(2); // Cleaner way to sleep for exactly 2 seconds
            continue;
        }
        DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses()Starting check for new process ", pid);
    }
    DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() finished CGI checking loop ", "");
}

// Potentially useful cleanup functions

// WebService::deleteFromPfdsVecForCGI(proc.output_pipe);

// Server *server_obj = WebService::fd_to_server[WebService::pfds_vec[it2->fd].fd];
// DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Server *server_obj = WebService::fd_to_server[WebService::pfds_vec[it2->fd].fd]", pid);

// // WebService::sendResponse(WebService::pfds_vec[it2->fd].fd, (size_t &)it2->fd, *server_obj);
// // DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() sendResponse", pid);

// // WebService::closeConnection(WebService::pfds_vec[it2->fd].fd, (size_t &)it2->fd, *server_obj);
// // DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() closeConnection", pid);

// // WebService::deleteRequestObject(WebService::pfds_vec[it2->fd].fd, *server_obj);
// DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() deleteRequestObject", pid);

// WebService::deleteFromPfdsVecForCGI(WebService::pfds_vec[it2->fd].fd);
// DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() deleteFromPfdsVecForCGI", pid);

// WebService::cgi_fd_to_http_response.erase(WebService::pfds_vec[it2->fd].fd);
// DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() cgi_fd_to_http_response.erase", pid);

// delete server_obj;
// DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() delete server_obj;", pid);

// WebService::fd_to_server.erase(WebService::pfds_vec[it2->fd].fd);
// DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() fd_to_server.erase", pid);

bool CGI::isfdOpen(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}
