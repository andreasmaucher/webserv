#include "../../include/cgi.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/httpResponse.hpp"
#include "../../include/webService.hpp"

#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>

// Default constructor for the CGI class
CGI::CGI() : clientSocket(-1), scriptPath(""), method(""), queryString(""), requestBody("") {}

// Static method to check if a given path is a CGI request
// Returns true if the path contains "/cgi-bin/", ".py", or ".cgi"
bool CGI::isCGIRequest(const std::string &path)
{
    // First check if it's in the cgi-bin directory
    /* if (path.find("/cgi-bin/") == std::string::npos) {
        return false;
    } */
    size_t len = path.length();
    return (len > 3 && path.substr(len - 3) == ".py");
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
    DEBUG_MSG("Project root", projectRoot);
    DEBUG_MSG("URI", uri);

    // Make sure cgi-bin exists in project root
    std::string relativePath = uri.substr(8); // removes "/cgi-bin"
    std::string fullPath = projectRoot + "/cgi-bin" + relativePath;
    DEBUG_MSG("Full resolved path", fullPath);
    return fullPath;
}

// Add this helper function
std::string CGI::constructErrorResponse(int status_code, const std::string &message)
{
    std::ostringstream response;
    response << "Status: " << status_code << "\r\n";
    response << "Content-Type: text/plain\r\n\r\n";
    response << message;
    return response.str();
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
    (void)fd;
    (void)response;
    try
    {
        std::string fullScriptPath = resolveCGIPath(request.uri);
        scriptPath = fullScriptPath;
        method = request.method;
        requestBody = request.body;

        // Separate checks for better error messages
        if (access(fullScriptPath.c_str(), F_OK) == -1)
        {
            request.error_code = 404; // Not Found
            request.body = constructErrorResponse(404, "Script not found: " + fullScriptPath);
            request.complete = true;
            return;
        }
        if (access(fullScriptPath.c_str(), X_OK) == -1)
        {
            request.error_code = 403; // Forbidden
            request.body = constructErrorResponse(403, "Script not executable: " + fullScriptPath);
            request.complete = true;
            return;
        }

        // char** env_array = setCGIEnvironment(request);
        std::string cgiOutput = executeCGI(fd, response, request);
        request.body = cgiOutput;
        request.complete = true;

        // Clean up environment variables
        /*  for (char **env = env_array; *env != NULL; env++) {
             free(*env);
         }
         delete[] env_array; */
    }
    catch (const std::exception &e)
    {
        DEBUG_MSG("CGI Error", e.what());
        throw;
    }
}

/*
Sets up the environment variables for the CGI script; second part converts http headers to env variables
*/
char **CGI::setCGIEnvironment(const HttpRequest &httpRequest) const
{
    // Create environment strings in the format "KEY=VALUE"
    std::vector<std::string> env_strings;

    // Add required CGI environment variables
    env_strings.push_back("REQUEST_METHOD=" + httpRequest.method);    // GET, POST, DELETE
    env_strings.push_back("QUERY_STRING=" + httpRequest.queryString); // everything after ? in URL

    // Convert content length to string using stringstream (C++98 compliant)
    std::stringstream ss;
    ss << httpRequest.body.length();
    env_strings.push_back("CONTENT_LENGTH=" + ss.str()); // length of POST data

    // Add Content-Type if present (crucial for multipart form data)
    std::map<std::string, std::string>::const_iterator it = httpRequest.headers.find("Content-Type");
    if (it != httpRequest.headers.end()) {
        env_strings.push_back("CONTENT_TYPE=" + it->second);
        DEBUG_MSG("CGI Content-Type", it->second);
    }

    env_strings.push_back("SCRIPT_NAME=" + httpRequest.uri);         // path to the CGI script
    env_strings.push_back("SERVER_PROTOCOL=" + httpRequest.version); // Usually HTTP/1.1

    // Convert strings to char* array
    char **env_array = new char *[env_strings.size() + 1];
    for (size_t i = 0; i < env_strings.size(); i++)
    {
        env_array[i] = strdup(env_strings[i].c_str());
    }
    env_array[env_strings.size()] = NULL; // NULL terminate the array

    return env_array;
}

/*
CGI LOGIC:
1. Creates communication channels (pipes) between server and CGI script
2. Forks into two processes (parent and child)
3. Child process executes the CGI script
4. Parent process reads the script's output
5. Returns the output as a string
*/
std::string CGI::executeCGI(int &fd, HttpResponse &response, HttpRequest &request)
{
    (void)fd;
    (void)response;
    DEBUG_MSG("=== CGI EXECUTION START ===", "");
    DEBUG_MSG("Server FD", fd);
    DEBUG_MSG("Script Path", scriptPath);
    DEBUG_MSG("Method", method);

    // Define pipes for stdin and stdout
    int pipe_in[2];  // Parent writes to pipe_in[1], child reads from pipe_in[0]
    int pipe_out[2]; // Child writes to pipe_out[1], parent reads from pipe_out[0]

    // Create input pipe
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
    DEBUG_MSG("Output pipe - Read FD", pipe_out[0]);
    DEBUG_MSG("Output pipe - Write FD", pipe_out[1]);

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

        // Execute the CGI script with our environment
        const char *pythonPath = PYTHON_PATH;
        char *const args[] = {
            const_cast<char *>(pythonPath),
            const_cast<char *>(scriptPath.c_str()),
            NULL};

        execve(pythonPath, args, env_array); // Use env_array instead of environ
        // Clean up env_array if execve fails
        for (char **env = env_array; *env != NULL; env++)
        {
            free(*env);
        }
        delete[] env_array;
        exit(EXIT_FAILURE);
    }

    // Parent process
    DEBUG_MSG("Parent: Child PID", pid);

    // Close unused file descriptors in parent
    close(pipe_in[0]);  // Close read end of input pipe
    close(pipe_out[1]); // Close write end of output pipe

    // If POST request, write the request body to the CGI's stdin
    if (method == "POST")
    {
        if (!requestBody.empty())
        {
            ssize_t bytes_written = write(pipe_in[1], requestBody.c_str(), requestBody.length());
            if (bytes_written == -1)
            {
                DEBUG_MSG("Parent: Failed to write POST data", strerror(errno));
                // Close file descriptors and throw
                close(pipe_in[1]);
                close(pipe_out[0]);
                throw std::runtime_error("Failed to write POST data to CGI stdin");
            }
            DEBUG_MSG("Parent: Wrote bytes to CGI stdin", bytes_written);
        }
        // After writing, close the write end to send EOF to CGI's stdin
        close(pipe_in[1]);
        DEBUG_MSG("Parent: Closed write end of input pipe after writing POST data", "");
    }
    else
    {
        // For GET requests, close the write end as there's no data to send
        close(pipe_in[1]);
        DEBUG_MSG("Parent: Closed write end of input pipe (no POST data)", "");
    }

    // Read CGI output from pipe_out[0]
    std::string cgi_output;
    char buffer[4096];
    ssize_t bytes_read;
    size_t total_bytes = 0;
    (void)total_bytes; //! not really used, take out just for debugging

    DEBUG_MSG("Parent: Reading CGI output from output pipe", "");
    while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
    {
        cgi_output.append(buffer, bytes_read);
        total_bytes += bytes_read;
        DEBUG_MSG("Parent: Read bytes from CGI", bytes_read);
    }

    if (bytes_read == -1)
    {
        DEBUG_MSG("Parent: Error reading CGI output", strerror(errno));
        // Close output pipe before throwing
        close(pipe_out[0]);
        throw std::runtime_error("Error reading CGI output");
    }

    DEBUG_MSG("Parent: Total bytes read", total_bytes);
    close(pipe_out[0]); // Close read end after reading

    // Wait for child process to finish
    int status;
    if (waitpid(pid, &status, 0) == -1)
    {
        DEBUG_MSG("Parent: waitpid failed", strerror(errno));
        throw std::runtime_error("waitpid failed");
    }

    if (WIFEXITED(status))
    {
        DEBUG_MSG("Parent: Child exit status", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        DEBUG_MSG("Parent: Child killed by signal", WTERMSIG(status));
    }
    else
    {
        DEBUG_MSG("Parent: Child exit status", "abnormal");
    }

    // Assign CGI output to request.body
    response.body = cgi_output;
    DEBUG_MSG("Parent: Response body size", response.body.length());
    DEBUG_MSG("=== CGI EXECUTION END ===", "");
    return cgi_output;
}
