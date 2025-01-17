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
bool CGI::isCGIRequest(const std::string& path) {
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
std::string CGI::resolveCGIPath(const std::string& uri) {
    char buffer[PATH_MAX];
    if (getcwd(buffer, PATH_MAX) == NULL) {
        throw std::runtime_error("Failed to get current working directory");
    }
    std::string projectRoot = std::string(buffer);
    std::cout << "Project root: " << projectRoot << std::endl;
    std::cout << "URI: " << uri << std::endl;
    
    // Make sure cgi-bin exists in your project root
    std::string relativePath = uri.substr(8);  // removes "/cgi-bin"
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
void CGI::handleCGIRequest(HttpRequest& request) {
    std::cerr << "1. Starting handleCGIRequest..." << std::endl;
    try {
        std::string fullScriptPath = resolveCGIPath(request.uri);
        scriptPath = fullScriptPath;
        if (access(fullScriptPath.c_str(), F_OK | X_OK) == -1) {
            throw std::runtime_error("Script not found or not executable: " + fullScriptPath);
        }
        setCGIEnvironment(request);
        std::string cgiOutput = executeCGI();
        request.body = cgiOutput;
        request.complete = true;
    } 
    catch (const std::exception& e) {
        std::cerr << "CGI Error: " << e.what() << std::endl;
        throw;
    }
}

/* 
Sets up the environment variables for the CGI script; second part converts http headers to env variables 
*/
void CGI::setCGIEnvironment(const HttpRequest& httpRequest) const
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
    for (it = httpRequest.headers.begin(); it != httpRequest.headers.end(); ++it) {
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
std::string CGI::executeCGI() {
    std::cerr << "A. Starting executeCGI..." << std::endl;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        throw std::runtime_error("Pipe creation failed");
    }
    std::cerr << "B. Pipe created, forking..." << std::endl;
    pid_t pid = fork();
    std::cerr << "C. Fork result: " << pid << std::endl;
    if (pid == -1) {
        throw std::runtime_error("Fork failed");
    }
    if (pid == 0) {  // Child process
        std::cerr << "D. Child process starting..." << std::endl;
        std::cerr << "Script path: " << scriptPath << std::endl;
        close(pipefd[0]);  // Close read end
        dup2(pipefd[1], STDOUT_FILENO);  // Redirect stdout to pipe
        close(pipefd[1]);
        // Create pipe for POST data
        int post_pipe[2];
        if (pipe(post_pipe) == -1) {
            std::cerr << "POST pipe creation failed" << std::endl;
            exit(1);
        }
        // Write POST data to pipe, GET requests have an empty body (pipe will be created but not written to)
        if (!requestBody.empty()) {
            write(post_pipe[1], requestBody.c_str(), requestBody.length());
        }
        close(post_pipe[1]);
        // Redirect stdin to read end of POST pipe
        dup2(post_pipe[0], STDIN_FILENO);
        close(post_pipe[0]);
        // Execute the script
        const char* pythonPath = "/usr/bin/python3";
        char *const args[] = {
            const_cast<char*>(pythonPath),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };
        execve(pythonPath, args, environ);
        // If we get here, execve failed
        std::cerr << "execve failed: " << strerror(errno) << std::endl;
        exit(1);
    }
    
    // Parent process
    std::cout << "Parent process waiting (Child PID: " << pid << ")" << std::endl;
    
    // Close write end
    close(pipefd[1]);
    
    // Read response with timeout
    std::string response;
    char buffer[4096];
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(pipefd[0], &readfds);
    tv.tv_sec = 5;  // 5 second timeout
    tv.tv_usec = 0;

    while (1) {
        int ready = select(pipefd[0] + 1, &readfds, NULL, NULL, &tv); // checks if data is available to read
        if (ready == -1) {
            perror("select failed");
            break;
        }
        if (ready == 0) {
            std::cout << "Read timeout" << std::endl;
            break;
        }
        
        ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) break;
        
        buffer[bytes_read] = '\0';
        response += buffer;
    }

    // Close read end
    close(pipefd[0]);

    // Wait for child process
    int status;
    waitpid(pid, &status, 0);
    std::cout << "Child process exited with status: " << status << std::endl;
    
    if (response.empty()) {
        std::cout << "No response received from CGI script" << std::endl;
        return "Status: 500\r\nContent-Type: text/plain\r\n\r\nNo response from CGI script";
    }
    std::cout << "=== CGI Execution End ===" << std::endl;
    return response;
}