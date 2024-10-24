#include "cgi.hpp"

// Constructor for the CGI class
// Initializes all member variables with the provided parameters
CGI::CGI(int clientSocket, const std::string& scriptPath, const std::string& method,
         const std::string& queryString, const std::string& requestBody)
    : clientSocket(clientSocket), scriptPath(scriptPath), method(method),
      queryString(queryString), requestBody(requestBody) {}

//! probably needs to go into the parser, not used in my code
// Static method to check if a given path is a CGI request
// Returns true if the path contains "/cgi-bin/", ".py", or ".cgi"
bool CGI::isCGIRequest(const std::string& path) {
    return path.find("/cgi-bin/") != std::string::npos || 
           path.find(".py") != std::string::npos || 
           path.find(".cgi") != std::string::npos;
}

// Main method to handle a CGI request
// it was determined before if CGI request or not
// Sets up the environment, executes the CGI script and sends the response
void CGI::handleCGIRequest() {
    setCGIEnvironment();
    /*
        To understand what will be returned if execve is successfully called:
        - response will not be directly set by the execve call, since it never returns
        - instead executeCGI() function forks the current process before calling execve
        - The parent process waits for the child to complete and captures its output.
        - this output is then returned by executeCGI() and set to response
        ->>> response string contains the output of the CGI script, not the return value of execve itself
    */
    std::string response = executeCGI();
    sendResponse(response);
}

// Sets up the environment variables for the CGI script
void CGI::setCGIEnvironment() const {
    setenv("REQUEST_METHOD", method.c_str(), 1);
    setenv("QUERY_STRING", queryString.c_str(), 1);
    setenv("CONTENT_LENGTH", std::to_string(requestBody.length()).c_str(), 1);
    setenv("SCRIPT_NAME", scriptPath.c_str(), 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
}


// Executes the CGI script and handles its output
std::string CGI::executeCGI() {
    int pipefd[2];   // Pipe to capture the CGI output
    pipe(pipefd);    // Create a pipe (pipefd[0] for reading, pipefd[1] for writing)

    /* 
        - in a parent processfork() always returns a positive integer (pid of the child process)
        - in the child process, fork() returns 0
    */
    pid_t pid = fork();  // Fork a new process to run CGI script

    if (pid == 0) {  // Child process (executes the CGI script)
        close(pipefd[0]);  // Close the reading end of the pipe in the child process

        // Redirect the CGI script's stdout to the pipe's writing end
        dup2(pipefd[1], STDOUT_FILENO);

        // If it's a POST request, redirect the stdin to handle the request body
        if (!requestBody.empty()) {
            dup2(pipefd[1], STDIN_FILENO);
        }

        // Execute the CGI script using exec (replace the child process with the CGI script)
        /*
            args array to hold the arguments for the CGI script;
            first argument: script path, second argument: NULL (indicating the end of the argument list)
            const cast: execve expects non-const array
            c_str: because execve is a c function and expects a c-style string
        */
        char *const args[] = {const_cast<char*>(scriptPath.c_str()), NULL};
        // NULL would normally be envs but here just means use the current environment
        execve(scriptPath.c_str(), args, NULL);

        // If execve fails, exit the child process (if successful, execve never returns)
        exit(1);
    } else if (pid > 0) {  // Parent process (captures output)
        close(pipefd[1]);  // Close the writing end in the parent

        // Wait for the child process (CGI script) to complete
        waitpid(pid, NULL, 0);

        return readFromPipe(pipefd[0]);
    }
    return "Status: 500 Internal Server Error\r\n\r\nCGI execution failed";
}

// Reads the CGI script's output from the pipe
std::string CGI::readFromPipe(int pipefd) const {
    std::stringstream ss;
    char buffer[1024];
    int bytesRead;

    while ((bytesRead = read(pipefd, buffer, sizeof(buffer))) > 0) {
        ss.write(buffer, bytesRead);
    }

    close(pipefd);
    return ss.str();
}

// Sends the complete response (headers + body) to the client
void CGI::sendResponse(const std::string& response) const {
    std::string fullResponse = response;
    
    // Check if the CGI script provided headers
    if (fullResponse.find("Status: ") == std::string::npos &&
        fullResponse.find("Content-Type: ") == std::string::npos) {
        // If no headers, add default headers
        fullResponse = "Status: 200 OK\r\nContent-Type: text/html\r\n\r\n" + fullResponse;
    } else if (fullResponse.find("\r\n\r\n") == std::string::npos) {
        // If headers are present but no empty line, add it
        fullResponse = fullResponse + "\r\n";
    }

    // Send the full response to the client
    send(clientSocket, fullResponse.c_str(), fullResponse.length(), 0);
}
