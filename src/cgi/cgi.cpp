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

/*
standard way to pass environment variables to the child process
*/
extern char **environ;

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

    std::string relativePath = uri.substr(8); // removes "/cgi-bin"
    std::string fullPath = projectRoot + "/cgi-bin" + relativePath;
    DEBUG_MSG("Full resolved path", fullPath);
    return fullPath;
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
    (void)fd;
    (void)response;
    DEBUG_MSG("CGI Request Handler - Client FD", fd);
    DEBUG_MSG("CGI Request Handler - Method", request.method);
    DEBUG_MSG("CGI Request Handler - URI", request.uri);

    if (!isCGIRequest(request.uri))
    {
        DEBUG_MSG("Invalid CGI Request for URI", request.uri);
        throw std::runtime_error("Not a CGI request");
    }

    scriptPath = resolveCGIPath(request.uri);
    DEBUG_MSG("Resolved CGI Script Path", scriptPath);

    // Create pipes for communication
    int pipe_in[2], pipe_out[2];
    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0)
    {
        DEBUG_MSG("Pipe creation failed with errno", errno);
        throw std::runtime_error("Failed to create pipes");
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        DEBUG_MSG("Fork failed with errno", errno);
        throw std::runtime_error("Fork failed");
    }

    if (pid == 0)
    { // Child process
        DEBUG_MSG("Child Process - PID", getpid());

        // Redirect stdin/stdout
        if (dup2(pipe_in[0], STDIN_FILENO) < 0 ||
            dup2(pipe_out[1], STDOUT_FILENO) < 0)
        {
            DEBUG_MSG("dup2 failed with errno", errno);
            exit(1);
        }

        // Close unused pipe ends
        close(pipe_in[1]);
        close(pipe_out[0]);

        // Set up environment variables
        setCGIEnvironment(request);

        DEBUG_MSG("Executing CGI Script", scriptPath);
        execl(scriptPath.c_str(), scriptPath.c_str(), NULL);

        DEBUG_MSG("execl failed with errno", errno);
        exit(1);
    }

    // Parent process
    DEBUG_MSG("Parent Process - Child PID", pid);

    // Close unused pipe ends
    close(pipe_in[0]);
    close(pipe_out[1]);

    // Write POST data if necessary
    if (request.method == "POST")
    {
        DEBUG_MSG("Writing POST data, size", request.body.length());
        write(pipe_in[1], request.body.c_str(), request.body.length());
    }
    close(pipe_in[1]);

    // Read CGI output
    char buffer[4096];
    std::string cgi_output;
    ssize_t bytes_read;

    while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
    {
        DEBUG_MSG("Read from CGI output", bytes_read);
        cgi_output.append(buffer, bytes_read);
    }
    close(pipe_out[0]);

    // Wait for child process
    int status;
    waitpid(pid, &status, 0);
    DEBUG_MSG("CGI Process Exit Status", WEXITSTATUS(status));

    response.body = cgi_output;
    DEBUG_MSG("CGI Response Size", response.body.length());
}

/*
Sets up the environment variables for the CGI script; second part converts http headers to env variables
*/
void CGI::setCGIEnvironment(const HttpRequest &httpRequest) const
{
    setenv("REQUEST_METHOD", httpRequest.method.c_str(), 1);
    setenv("QUERY_STRING", httpRequest.queryString.c_str(), 1);
    std::ostringstream ss;
    ss << httpRequest.body.length();
    setenv("CONTENT_LENGTH", ss.str().c_str(), 1);
    setenv("SCRIPT_NAME", httpRequest.uri.c_str(), 1);
    setenv("SERVER_PROTOCOL", httpRequest.version.c_str(), 1);

    // Set content type from headers if available
    std::map<std::string, std::string>::const_iterator contentTypeIt = httpRequest.headers.find("Content-Type");
    if (contentTypeIt != httpRequest.headers.end())
    {
        setenv("CONTENT_TYPE", contentTypeIt->second.c_str(), 1);
    }
    std::map<std::string, std::string>::const_iterator it;
    for (it = httpRequest.headers.begin(); it != httpRequest.headers.end(); ++it)
    {
        std::string envName = "HTTP_" + it->first;
        std::replace(envName.begin(), envName.end(), '-', '_');
        std::transform(envName.begin(), envName.end(), envName.begin(), ::toupper);
        setenv(envName.c_str(), it->second.c_str(), 1);
    }
}

/*
CGI LOGIC:
1. Creates communication channels (pipes) between server and CGI script
2. Forks into two processes (parent and child)
3. Child process executes the CGI script
4. Parent process reads the script's output
5. Returns the output as a string
*/
std::string CGI::executeCGI(int &fd, HttpResponse &response)
{
    (void)fd;
    (void)response;
    DEBUG_MSG("=== CGI EXECUTION START ===", "");
    DEBUG_MSG("Server FD", fd);
    DEBUG_MSG("Script Path", scriptPath);
    DEBUG_MSG("Method", method);

    int pipe_in[2];  // Parent writes to pipe_in[1], child reads from pipe_in[0]
    int pipe_out[2]; // Child writes to pipe_out[1], parent reads from pipe_out[0]

    if (pipe(pipe_in) == -1)
    {
        DEBUG_MSG("Pipe creation for stdin failed", strerror(errno));
        throw std::runtime_error("Pipe creation for stdin failed");
    }
    DEBUG_MSG("Created input pipe - Read FD", pipe_in[0]);
    DEBUG_MSG("Created input pipe - Write FD", pipe_in[1]);

    if (pipe(pipe_out) == -1)
    {
        DEBUG_MSG("Pipe creation for stdout failed", strerror(errno));
        close(pipe_in[0]);
        close(pipe_in[1]);
        throw std::runtime_error("Pipe creation for stdout failed");
    }
    DEBUG_MSG("Created output pipe - Read FD", pipe_out[0]);
    DEBUG_MSG("Created output pipe - Write FD", pipe_out[1]);

    pid_t pid = fork();
    if (pid == -1)
    {
        DEBUG_MSG("Fork failed", strerror(errno));
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        throw std::runtime_error("Fork failed");
    }

    if (pid == 0)
    { // Child process
        DEBUG_MSG("Child process started", getpid());

        if (dup2(pipe_in[0], STDIN_FILENO) == -1)
        {
            DEBUG_MSG("Child: dup2 for stdin failed", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (dup2(pipe_out[1], STDOUT_FILENO) == -1)
        {
            DEBUG_MSG("Child: dup2 for stdout failed", strerror(errno));
            exit(EXIT_FAILURE);
        }

        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);

        DEBUG_MSG("Child: Executing CGI script", scriptPath);
        const char *pythonPath = "/usr/bin/python3";
        char *const args[] = {const_cast<char *>(pythonPath), const_cast<char *>(scriptPath.c_str()), NULL};
        execve(pythonPath, args, environ);

        DEBUG_MSG("Child: execve failed", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Parent process
    DEBUG_MSG("Parent: Child PID", pid);

    close(pipe_in[0]);
    close(pipe_out[1]);

    if (method == "POST" && !requestBody.empty())
    {
        ssize_t bytes_written = write(pipe_in[1], requestBody.c_str(), requestBody.length());
        if (bytes_written == -1)
        {
            DEBUG_MSG("Parent: Failed to write POST data", strerror(errno));
            close(pipe_in[1]);
            close(pipe_out[0]);
            throw std::runtime_error("Failed to write POST data to CGI stdin");
        }
        DEBUG_MSG("Parent: Wrote bytes to CGI stdin", bytes_written);
    }
    close(pipe_in[1]);
    DEBUG_MSG("Parent: Closed write end of input pipe", method == "POST" ? "after POST data" : "no POST data");

    std::string cgi_output;
    char buffer[4096];
    ssize_t bytes_read;
    size_t total_bytes = 0;
    (void)total_bytes;

    DEBUG_MSG("Parent: Reading CGI output", "started");
    while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
    {
        cgi_output.append(buffer, bytes_read);
        total_bytes += bytes_read;
        DEBUG_MSG("Parent: Read bytes from CGI output", bytes_read);
    }

    if (bytes_read == -1)
    {
        DEBUG_MSG("Parent: Error reading CGI output", strerror(errno));
        close(pipe_out[0]);
        throw std::runtime_error("Error reading CGI output");
    }

    DEBUG_MSG("Parent: Total bytes read", total_bytes);
    close(pipe_out[0]);

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

    response.body = cgi_output;
    DEBUG_MSG("Parent: Response body size", response.body.length());
    DEBUG_MSG("=== CGI EXECUTION END ===", "");

    return cgi_output;
}