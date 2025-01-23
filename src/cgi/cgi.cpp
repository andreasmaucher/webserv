#include "../../include/cgi.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/httpResponse.hpp"

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
    return (len > 3 && path.substr(len - 3) == ".py") ||
           (len > 4 && path.substr(len - 4) == ".php");
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

    // Make sure cgi-bin exists in your project root
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
void CGI::handleCGIRequest(HttpRequest &request)
{
    std::cerr << "Starting CGI handling for URI: " << request.uri << std::endl;
    try
    {
        // Validate request state
        if (request.uri.empty() || request.method.empty())
        {
            throw std::runtime_error("Invalid request state for CGI");
        }

        std::string fullScriptPath = resolveCGIPath(request.uri);
        scriptPath = fullScriptPath;
        
        // Add extra validation
        if (access(fullScriptPath.c_str(), F_OK) == -1)
        {
            request.error_code = 404;
            throw std::runtime_error("Script not found: " + fullScriptPath);
        }
        if (access(fullScriptPath.c_str(), X_OK) == -1)
        {
            request.error_code = 403;
            throw std::runtime_error("Script not executable: " + fullScriptPath);
        }

        setCGIEnvironment(request);
        std::string cgiOutput = executeCGI();
        
        if (cgiOutput.empty())
        {
            request.error_code = 502;
            throw std::runtime_error("CGI script returned empty response");
        }

        request.body = cgiOutput;
        request.complete = true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "CGI Error: " << e.what() << std::endl;
        if (request.error_code == 0)
        { // Only set if not already set
            request.error_code = 500;
        }
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
    setenv("CONTENT_TYPE", httpRequest.contentType.c_str(), 1);
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
std::string CGI::executeCGI()
{
    int stdout_pipe[2];
    int stdin_pipe[2];
    
    // Create pipes
    if (pipe(stdout_pipe) == -1 || pipe(stdin_pipe) == -1) {
        throw std::runtime_error("Pipe creation failed: " + std::string(strerror(errno)));
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        throw std::runtime_error("Fork failed: " + std::string(strerror(errno)));
    }

    if (pid == 0) { // Child process
        try {
            // Close unused pipe ends
            close(stdout_pipe[0]);  // Close read end of stdout pipe
            close(stdin_pipe[1]);   // Close write end of stdin pipe

            // Redirect stdin and stdout
            if (dup2(stdin_pipe[0], STDIN_FILENO) == -1 ||
                dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
                throw std::runtime_error("Failed to redirect IO");
            }

            // Close original file descriptors
            close(stdin_pipe[0]);
            close(stdout_pipe[1]);

            // Execute the script
            const char *pythonPath = "/usr/bin/python3";  // Use system python path
            char *const args[] = {
                const_cast<char *>(pythonPath),
                const_cast<char *>(scriptPath.c_str()),
                NULL
            };

            execve(pythonPath, args, environ);
            throw std::runtime_error("execve failed: " + std::string(strerror(errno)));
        }
        catch (const std::exception& e) {
            std::cerr << "Child process error: " << e.what() << std::endl;
            _exit(1);  // Use _exit in child process
        }
    }

    // Parent process
    close(stdout_pipe[1]);  // Close write end of stdout pipe
    close(stdin_pipe[0]);   // Close read end of stdin pipe

    // Write request body to script's stdin if present
    if (!requestBody.empty()) {
        write(stdin_pipe[1], requestBody.c_str(), requestBody.length());
    }
    close(stdin_pipe[1]);  // Close stdin pipe after writing

    // Read response with timeout
    std::string response;
    char buffer[4096];
    fd_set readfds;
    struct timeval tv;
    int max_retries = 3;
    int retries = 0;

    while (retries < max_retries) {
        FD_ZERO(&readfds);
        FD_SET(stdout_pipe[0], &readfds);

        tv.tv_sec = 5;  // Increased timeout
        tv.tv_usec = 0;

        int ready = select(stdout_pipe[0] + 1, &readfds, NULL, NULL, &tv);
        if (ready == -1) {
            if (errno == EINTR) {
                continue;  // Retry on interrupt
            }
            close(stdout_pipe[0]);
            throw std::runtime_error("select() failed: " + std::string(strerror(errno)));
        }
        if (ready == 0) {
            retries++;
            continue;
        }

        ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            if (bytes_read == 0) break;  // EOF
            if (errno == EINTR) continue;  // Retry on interrupt
            throw std::runtime_error("read() failed: " + std::string(strerror(errno)));
        }

        buffer[bytes_read] = '\0';
        response += buffer;
    }

    close(stdout_pipe[0]);

    // Wait for child process
    int status;
    pid_t wait_result;
    do {
        wait_result = waitpid(pid, &status, WNOHANG);
        if (wait_result == -1) {
            throw std::runtime_error("waitpid() failed: " + std::string(strerror(errno)));
        }
        if (wait_result == 0) {
            kill(pid, SIGTERM);
            usleep(100000);  // Wait 100ms
        }
    } while (wait_result == 0);

    // Check if script executed successfully
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error("CGI script failed with status: " + std::to_string(WEXITSTATUS(status)));
    }

    if (response.empty()) {
        return "Status: 500\r\nContent-Type: text/plain\r\n\r\nNo response from CGI script";
    }

    return response;
}