#include "../../include/cgi.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/httpResponse.hpp"
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
    std::cout << "Project root: " << projectRoot << std::endl;
    std::cout << "URI: " << uri << std::endl;

    // Make sure cgi-bin exists in project root
    std::string relativePath = uri.substr(8); // removes "/cgi-bin"
    std::string fullPath = projectRoot + "/cgi-bin" + relativePath;
    std::cout << "Full resolved path: " << fullPath << std::endl;
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
    std::cerr << "1. Starting handleCGIRequest..." << std::endl;
    try
    {
        std::string fullScriptPath = resolveCGIPath(request.uri);
        scriptPath = fullScriptPath;
        method = request.method;
        requestBody = request.body;
        if (access(fullScriptPath.c_str(), F_OK | X_OK) == -1)
        {
            throw std::runtime_error("Script not found or not executable: " + fullScriptPath);
        }
        setCGIEnvironment(request);
        std::string cgiOutput = executeCGI(fd, response);
        request.body = cgiOutput;
        request.complete = true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "CGI Error: " << e.what() << std::endl;
        throw;
    }
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
    std::cerr << "\n=== CGI EXECUTION START ===\n";
    std::cerr << "Server FD: " << fd << std::endl;
    std::cerr << "Script Path: " << scriptPath << std::endl;
    std::cerr << "Method: " << method << std::endl;

    // Define pipes for stdin and stdout
    int pipe_in[2];  // Parent writes to pipe_in[1], child reads from pipe_in[0]
    int pipe_out[2]; // Child writes to pipe_out[1], parent reads from pipe_out[0]

    // Create input pipe
    if (pipe(pipe_in) == -1)
    {
        std::cerr << "Pipe creation for stdin failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Pipe creation for stdin failed");
    }
    std::cerr << "Created input pipe - Read FD: " << pipe_in[0] << ", Write FD: " << pipe_in[1] << std::endl;

    // Create output pipe
    if (pipe(pipe_out) == -1)
    {
        std::cerr << "Pipe creation for stdout failed: " << strerror(errno) << std::endl;
        // Close previously opened pipe_in before throwing
        close(pipe_in[0]);
        close(pipe_in[1]);
        throw std::runtime_error("Pipe creation for stdout failed");
    }
    std::cerr << "Created output pipe - Read FD: " << pipe_out[0] << ", Write FD: " << pipe_out[1] << std::endl;

    pid_t pid = fork();
    if (pid == -1)
    {
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
        // Close all opened file descriptors before throwing
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        throw std::runtime_error("Fork failed");
    }

    if (pid == 0)
    { // Child process
        std::cerr << "Child process started" << std::endl;

        // Redirect stdin to pipe_in[0]
        if (dup2(pipe_in[0], STDIN_FILENO) == -1)
        {
            std::cerr << "Child: dup2 for stdin failed: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to pipe_out[1]
        if (dup2(pipe_out[1], STDOUT_FILENO) == -1)
        {
            std::cerr << "Child: dup2 for stdout failed: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }

        // Close unused file descriptors in child
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);

        // Set environment variables (already done in setCGIEnvironment())

        // Execute the CGI script
        std::cerr << "Child: Executing CGI script: " << scriptPath << std::endl;
        const char *pythonPath = "/usr/bin/python3";
        char *const args[] = {const_cast<char *>(pythonPath), const_cast<char *>(scriptPath.c_str()), NULL};
        execve(pythonPath, args, environ);

        // If execve fails
        std::cerr << "Child: execve failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Parent process
    std::cerr << "Parent: Child PID: " << pid << std::endl;

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
                std::cerr << "Parent: Failed to write POST data to CGI stdin: " << strerror(errno) << std::endl;
                // Close file descriptors and throw
                close(pipe_in[1]);
                close(pipe_out[0]);
                throw std::runtime_error("Failed to write POST data to CGI stdin");
            }
            std::cerr << "Parent: Wrote " << bytes_written << " bytes to CGI stdin" << std::endl;
        }
        // After writing, close the write end to send EOF to CGI's stdin
        close(pipe_in[1]);
        std::cerr << "Parent: Closed write end of input pipe after writing POST data" << std::endl;
    }
    else
    {
        // For GET requests, close the write end as there's no data to send
        close(pipe_in[1]);
        std::cerr << "Parent: Closed write end of input pipe (no POST data)" << std::endl;
    }

    // Read CGI output from pipe_out[0]
    std::string cgi_output;
    char buffer[4096];
    ssize_t bytes_read;
    size_t total_bytes = 0;

    std::cerr << "Parent: Reading CGI output from output pipe" << std::endl;
    while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
    {
        cgi_output.append(buffer, bytes_read);
        total_bytes += bytes_read;
        std::cerr << "Parent: Read " << bytes_read << " bytes from CGI output" << std::endl;
    }

    if (bytes_read == -1)
    {
        std::cerr << "Parent: Error reading CGI output: " << strerror(errno) << std::endl;
        // Close output pipe before throwing
        close(pipe_out[0]);
        throw std::runtime_error("Error reading CGI output");
    }

    std::cerr << "Parent: Finished reading CGI output (" << total_bytes << " bytes)" << std::endl;
    close(pipe_out[0]); // Close read end after reading

    // Wait for child process to finish
    int status;
    if (waitpid(pid, &status, 0) == -1)
    {
        std::cerr << "Parent: waitpid failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("waitpid failed");
    }

    if (WIFEXITED(status))
    {
        std::cerr << "Parent: Child exited with status: " << WEXITSTATUS(status) << std::endl;
    }
    else if (WIFSIGNALED(status))
    {
        std::cerr << "Parent: Child was killed by signal: " << WTERMSIG(status) << std::endl;
    }
    else
    {
        std::cerr << "Parent: Child did not exit normally" << std::endl;
    }

    // Assign CGI output to request.body
    response.body = cgi_output;
    std::cerr << "Parent: Assigned CGI output to request.body" << std::endl;

    std::cerr << "=== CGI EXECUTION END ===\n"
              << std::endl;
    return cgi_output;
}