#include "../../include/cgi.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/httpResponse.hpp"
#include "../../include/webService.hpp"
#include "../../include/responseHandler.hpp"

// Default constructor for the CGI class
CGI::CGI() : clientSocket(-1), scriptPath(""), method(""), queryString(""), requestBody("") {}

// Static method to check if a given path is a CGI request
// Returns true if the path contains "/cgi-bin/", ".py", or ".cgi"
bool CGI::isCGIRequest(const std::string &path)
{
    // Check if path starts with "/cgi-bin/" -> make sure no random or invalid directories are before cgi-bin
    if (path.find("/cgi-bin/") != 0) {
        return false; // Not a valid CGI path - cgi-bin is not at the beginning
    }
    size_t start = path.find("/cgi-bin/") + 9; // +9 to skip "/cgi-bin/"
    size_t end = path.find('/', start);        // Find next '/' or end of string
    if (end == std::string::npos)
    {
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

    return fullPath;
}

// helper function to construct error response
std::string CGI::constructErrorResponse(int status_code, const std::string &message)
{
    CGI cgi;
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
    case 504:
        return "Gateway Timeout";
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

    std::string pathInfo = uri.substr(scriptEnd);
    // Remove any leading slashes
    pathInfo = pathInfo.substr(pathInfo.find_first_not_of('/'));

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

    response.is_cgi_response = true; // used to differentiate between cgi and static error pages

    DEBUG_MSG_2("CGI::handleCGIRequest receiving FD is  ", fd);

    std::string fullScriptPath = resolveCGIPath(request.uri);
    scriptPath = fullScriptPath;
    method = request.method;
    requestBody = request.body;

    executeCGI(fd, response, request);
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
    // Extract PATH_INFO (everything after .py)
    size_t scriptEnd = httpRequest.uri.find(".py") + 3;
    std::string pathInfo = "";
    if (scriptEnd < httpRequest.uri.length())
    {
        pathInfo = httpRequest.uri.substr(scriptEnd);
    }
    env_strings.push_back("PATH_INFO=" + pathInfo);

    // Set SCRIPT_NAME (the path to the script itself)
    std::string scriptName = httpRequest.uri.substr(0, scriptEnd);
    env_strings.push_back("SCRIPT_NAME=" + scriptName);

    // Add debugging
    DEBUG_MSG("Setting PATH_INFO to", pathInfo);
    DEBUG_MSG("Setting SCRIPT_NAME to", scriptName);

    // Convert strings to char* array
    char **env_array = new char *[env_strings.size() + 1];
    for (size_t i = 0; i < env_strings.size(); i++)
    {
        env_array[i] = strdup(env_strings[i].c_str());
        DEBUG_MSG(" env_array[i] is ", env_array[i]);
    }
    env_array[env_strings.size()] = NULL;
    return env_array;
}

void CGI::postRequest(int pipe_in[2])
{
    if (method == "POST" && !requestBody.empty())
    {
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
                close(pipe_in[1]);
                throw std::runtime_error("Failed to write to CGI input pipe");
            }

            total_written += written;
            remaining -= written;
        } 
        close(pipe_in[1]); // Close write end immediately for non-POST requests
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
        //  Close previously opened pipe_in before throwing
        close(pipe_in[0]);
        close(pipe_in[1]);
        throw std::runtime_error("Pipe creation for stdout failed");
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        DEBUG_MSG("Fork failed", strerror(errno));
        //  Close all opened file descriptors before throwing
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
            throw std::runtime_error("dup2 for stdin failed");
        }
        close(pipe_in[0]); // Child closes its copy after redirecting
        close(pipe_in[1]); // Child closes write end of input pipe

        // Redirect stdout to pipe_out[1]
        if (dup2(pipe_out[1], STDOUT_FILENO) == -1)
        {
            DEBUG_MSG("Child: dup2 for stdout failed", strerror(errno));
            throw std::runtime_error("dup2 for stdout failed");
        }

        // Close unused file descriptors in child
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
    int pipe_in[2];  // Parent writes to pipe_in[1], child reads from pipe_in[0]
    int pipe_out[2]; // Child writes to pipe_out[1], parent reads from pipe_out[0]

    pid_t pid = runChildCGI(pipe_in, pipe_out, request);

    DEBUG_MSG_2("---------------->CGI: executeCGI: new CGI process added: PID ", pid);
    DEBUG_MSG_2("---------------->CGI: executeCGI: new CGI process added: fd ", fd);
    if (pid > 0)
    {
        // Close unused file descriptors in parent
        close(pipe_in[0]);  // Close read end of input pipe
        close(pipe_out[1]); // Close write end of output pipe

        if (request.method == "POST")
            postRequest(pipe_in);
        else
            close(pipe_in[1]);

        // Add process to tracking map right after fork
        addProcess(pid, pipe_out[0], fd, request, &response);
        DEBUG_MSG_3("CGI:WebService::fd_to_server.erase(fd); ", fd);

        WebService::fd_to_server.erase(fd);

        // careful to use copies of fd, not references, otherwise reserve() will mess up the vector
        int output_pipe = pipe_out[0];  // Store a copy of the value
        int client_fd = fd;

        // Store the fd value before adding to vector
        int output_pipe_fd = output_pipe;
        WebService::addToPfdsVector(output_pipe_fd, true);
        // Use the local copy instead of the reference that might be invalidated
        output_pipe = output_pipe_fd;

        WebService::setPollfdEventsToIn(output_pipe);
        WebService::setPollfdEventsToOut(client_fd);

        WebService::fd_to_server.erase(output_pipe);

        DEBUG_MSG_2("CGI: WebService::addToPfdsVector added fd: ", output_pipe_fd);
        WebService::cgi_fd_to_http_response[output_pipe] = &response;
        WebService::cgi_fd_to_http_response[client_fd] = &response;

        DEBUG_MSG_3("CGI: WebService:: added new process at response_fd ", client_fd);

        DEBUG_MSG_3("CGI: WebService:: same proces  at output_pipe fd ", output_pipe);

        WebService::printPollFdStatus(WebService::findPollFd(output_pipe));

        DEBUG_MSG_2(" WebService::cgi_fd_to_http_response[pollfd_obj.fd] added fd: ", output_pipe);
    }
}

std::map<pid_t, CGI::CGIProcess> CGI::running_processes;

void CGI::addProcess(pid_t pid, int output_pipe, int response_fd, HttpRequest &req, HttpResponse *response)
{
    CGIProcess proc;
    proc.last_update_time = time(NULL);
    proc.output_pipe = output_pipe;
    proc.request = &req;
    proc.response_fd = response_fd;
    proc.response = response;

    DEBUG_MSG_2("Adding new CGI process PID", pid);
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
}

void CGI::killCGI(pid_t pid, CGIProcess &proc)
{
    // Check for timeout
    time_t now = time(NULL);
    if ((now - proc.last_update_time) > CGI_TIMEOUT)
    {
        DEBUG_MSG_2("CGI timeout reached for pid", pid);
        close(proc.output_pipe);
        WebService::deleteFromPfdsVecForCGI(proc.output_pipe);
        
        // Send timeout response to client before closing
        
        // Kill the process
        kill(pid, SIGKILL);
        // Wait for it to be reaped
        waitpid(pid, NULL, 0);
        // Same cleanup as above
        proc.process_finished = true;
    }
}

void CGI::readCGI(pid_t pid, CGIProcess &proc)
{
    (void)pid;
    ssize_t bytes_read;
    char buffer[MAX_CGI_BODY_SIZE];
    WebService::printPollFdStatus(WebService::findPollFd(proc.output_pipe));

    DEBUG_MSG_3("READ started at readFromCGI", proc.output_pipe);
    // sleep(1);
    if ((bytes_read = read(proc.output_pipe, buffer, sizeof(buffer))) > 0)
    {
        proc.last_update_time = time(NULL);
        DEBUG_MSG_3("READ at readFromCGI, read bytes: ", bytes_read);
        proc.response->body.append(buffer, bytes_read);
        proc.process_finished = false;

        return;
    }
    else if (bytes_read == -1)
    {
        DEBUG_MSG_3("READ at readFromCGI, read bytes: ", bytes_read);
        DEBUG_MSG_2("CGI: Read error on pipe", proc.output_pipe);
        // Mark the process as finished with an error.
        proc.process_finished = true;
        proc.finished_success = false;
        if (errno == ECHILD)
        {
            proc.last_update_time = time(NULL);
            proc.process_finished = true;
            proc.finished_success = true;
            proc.response->complete = true;
            proc.response->status_code = 200;
            proc.response->version = "HTTP/1.1";
            proc.response->reason_phrase = "OK";
            DEBUG_MSG_2("readCGI() read : ", proc.response->body);

            DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child finished, End of file reached", "");
        }

        // Close the pipe and remove it from your poll vector.
        close(proc.output_pipe);
        WebService::deleteFromPfdsVecForCGI(proc.output_pipe);
    }
    else if (bytes_read == 0)
    {
        proc.last_update_time = time(NULL);
        proc.process_finished = true;
        proc.finished_success = true;
        proc.response->complete = true;
        proc.response->status_code = 200;
        proc.response->version = "HTTP/1.1";
        proc.response->reason_phrase = "OK";
        DEBUG_MSG_2("readCGI() read : ", proc.response->body);
        DEBUG_MSG_2("Webservice::CGI::checkRunningProcesses() Child finished, End of file reached", "");
    }
    // Process cleanup

    if (close(proc.output_pipe) != 0)
    {
        DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() proc.response_fd pipe could not be closed  ", proc.output_pipe);
    }
    else
    {
        DEBUG_MSG_2("-----------> Webservice::CGI::checkRunningProcesses() proc.response_fd closed  ", proc.output_pipe);
    }

    pid_t result = waitpid(pid, &proc.status, WNOHANG);
    if (result > 0)
    {
        DEBUG_MSG_3("-----------> CGI::readCGI() waitpid returned :  ", proc.output_pipe);

        // Child has exited, status is valid
        if (WIFEXITED(proc.status) && WEXITSTATUS(proc.status) == 0)
        {
            // Process exited normally with status 0
            DEBUG_MSG_3("-----------> CGI::readCGI() waitpid returned success on exit  ", proc.output_pipe);

            proc.finished_success = true;
        }
        else
        {
            killCGI(pid, proc);
            DEBUG_MSG_3("-----------> CGI::readCGI() waitpid DIDNT return success on exit  WIFEXITED(proc.status)", WIFEXITED(proc.status));
            DEBUG_MSG_3("-----------> CGI::readCGI() waitpid DIDNT return success on exit WEXITSTATUS(proc.status)", WEXITSTATUS(proc.status));

            // Process exited abnormally or with non-zero status
            proc.finished_success = false;
        }
    }
    else if (result == 0)
    {
        // Child is still running
        killCGI(pid, proc);
        DEBUG_MSG_2("CGI: Process still running", pid);
    }
    else if (result == -1)
    {
        DEBUG_MSG_2("CGI: waitpid error", "");
    }
    WebService::deleteFromPfdsVecForCGI(proc.output_pipe);
    DEBUG_MSG_3("------->WebService::cgi_fd_to_http_response.erase(proc.output_pipe);", proc.output_pipe);

    WebService::cgi_fd_to_http_response.erase(proc.output_pipe);
    WebService::setPollfdEventsToOut(proc.response_fd);
}

void CGI::printRunningProcesses()
{
    DEBUG_MSG_2("=== Printing running_processes map ===", "");
    for (std::map<pid_t, CGIProcess>::iterator it = running_processes.begin();
         it != running_processes.end(); ++it)
    {
        DEBUG_MSG_2("PID: ", it->first);
        DEBUG_MSG_2("Response FD: ", it->second.response_fd);
        DEBUG_MSG_2("Output Pipe: ", it->second.output_pipe);
        DEBUG_MSG_2("Process Finished: ", it->second.process_finished);
        DEBUG_MSG_2("Success: ", it->second.finished_success);
        DEBUG_MSG_2("Start Time: ", it->second.last_update_time);
        DEBUG_MSG_2("------------------------", "");
    }
}

// 1. Find correct CGI process using the fd. Either the process with the proc.output_pipe or proc.response_fd
// 2. If the process is proc.output_pipe - then read from it. Then waitpid and cleanup. Return back to main loop.
// 3. If the process if response_fd - then send response. Then cleanup. Return to main loop
void CGI::checkCGIProcess(int pfds_fd)
{
    if (running_processes.empty())
        return;

    DEBUG_MSG_2("entered CGI::checkCGIProcess(int pfds_fd)  ", pfds_fd);
    printRunningProcesses();

    // Find the matching process
    std::map<pid_t, CGIProcess>::iterator matching_it = running_processes.end();
    bool found_cgi = false;

    for (std::map<pid_t, CGIProcess>::iterator it = running_processes.begin();
         it != running_processes.end(); ++it)
    {
        pid_t current_pid = it->first;
        (void)current_pid;
        CGIProcess &current_proc = it->second;

        DEBUG_MSG_2("CGI::checkCGIProcess checking process PID", current_pid);
        DEBUG_MSG_2("CGI::checkRunningProcesses: proc.output_pipe is   ", current_proc.output_pipe);
        DEBUG_MSG_2("CGI::checkRunningProcesses: proc.response_fd is   ", current_proc.response_fd);

        if (pfds_fd == current_proc.output_pipe || pfds_fd == current_proc.response_fd)
        {
            matching_it = it;
            found_cgi = true;
            DEBUG_MSG_2("CGI::checkRunningProcesses: FOUND CGI process which wants to write to pipe or reply ", pfds_fd);
            break;
        }
    }

    // If no match was found, return
    if (!found_cgi || matching_it == running_processes.end())
    {
        DEBUG_MSG_2("CGI::checkCGIProcess No matching process found for fd", pfds_fd);
        return;
    }

    // Now use the correct iterator
    pid_t pid = matching_it->first;
    CGIProcess &proc = matching_it->second;

    // Rest of your existing code using the correct proc reference
    if (pfds_fd == proc.output_pipe && found_cgi)
    {
        DEBUG_MSG_2("CGI::checkRunningProcesses: will try to read from CGI proceses  ", pfds_fd);
        killCGI(pid, proc);
        if (!proc.process_finished)
            readCGI(pid, proc);
        return;
    }
    if (pfds_fd == proc.response_fd && found_cgi && proc.finished_success)
    {
        DEBUG_MSG_2("CGI::checkRunningProcesses: will try to send CGI response ", pfds_fd);
        sendCGIResponse(proc);
        WebService::deleteFromPfdsVecForCGI(proc.response_fd);
        WebService::cgi_fd_to_http_response.erase(proc.response_fd);
        delete proc.response;
        running_processes.erase(matching_it);
        return;
    }
}


void CGI::checkAllCGIProcesses()
{
    time_t current_time = time(NULL);
    
    std::map<pid_t, CGIProcess>::iterator it = running_processes.begin();
    while (it != running_processes.end()) {
        pid_t pid = it->first;
        CGIProcess &proc = it->second;
        
        if ((current_time - proc.last_update_time) > CGI_TIMEOUT) {
            DEBUG_MSG("CGI timeout detected for pid", pid);
            
            // Force kill the process
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            
            // Close file descriptors
            if (proc.output_pipe > 0) 
                close(proc.output_pipe);
            if (proc.response_fd > 0) 
            {
                proc.response->status_code = 504;
                DEBUG_MSG_1("response.status_code ", proc.response->status_code);
                proc.response->close_connection = true;
                ResponseHandler::responseBuilder(*(proc.response));;
                std::string responseStr = proc.response->generateRawResponseStr();                
                ssize_t bytes_sent = send(proc.response_fd, responseStr.c_str(), responseStr.size(), 0);
                if (bytes_sent == -1) {
                    DEBUG_MSG("Error sending timeout response", strerror(errno));
                } else {
                    DEBUG_MSG_2("Sent timeout response, bytes:", bytes_sent);
                }
            }
            close(proc.response_fd);
            close(proc.output_pipe);
            if (proc.response)
                delete proc.response;            
            // Remove from tracking structures
            WebService::deleteFromPfdsVecForCGI(proc.output_pipe);
            WebService::deleteFromPfdsVecForCGI(proc.response_fd);
            WebService::cgi_fd_to_http_response.erase(proc.response_fd);
            WebService::cgi_fd_to_http_response.erase(proc.output_pipe);
            std::map<pid_t, CGIProcess>::iterator temp = it;
            ++it;
            running_processes.erase(temp);
        } else {
            ++it;
        }
    }
}
