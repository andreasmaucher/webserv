#include "cgi.hpp"
#include <unistd.h>     // for execve
#include <cstdlib>      // for environ

extern char **environ;  //! am I allowed to use this?

/*
    - have a parser object and a server_config object
    Parser object:
        - method: get / post / delete
        - path
        - protocol: http version
        - a map of headers (each header name is associated with its value)
        - body: body of the request may contain data if POST request
        - CGI: if cgi request this contains paramters from query string or body
        - if applicable upload information (upload filename)
        -> access to all these fields via getter functions

    Server_config object:
        - I need root: root directory where the php scripts are located
        - 
        


    addENVfunction:
        - extract info from parser object and formats them into env variable strings
        1. check if request body is empty
            - if empty set CONTENT_LENGTH to 0
            - else set CONTENT_LENGTH to length of request body
        2. determine http method (get.method)
            - assign corresponding value to method variable (GET, POST, DELETE)
        3. set upload path
            - add a dot to the requested path
        4. get CGI parameters:
            - CGI parameters are key-value pairs that are passed to a CGI script when it is executed
            - one function to retrieve CGI parameters from the map
            - convert the parameters into string format and add them to env table (format: KEY=VALUE)
            Example:
            request URL is http://example.com/script.php?username=john_doe&age=30, the CGI parameters would include:
                - username=john_doe
                - age=30
        ///? I take from Carina her HTTP Object with all fields
        ///? which fields exist in the cgi mapping?


*/

//! I need to fill all those fields!
// Constructor for the CGI class
// Initializes all member variables with the provided parameters
CGI::CGI(int clientSocket, const std::string& scriptPath, const std::string& method,
         const std::string& queryString, const std::string& requestBody)
    : clientSocket(clientSocket), scriptPath(scriptPath), method(method),
      queryString(queryString), requestBody(requestBody) {}

//! env setup still lacking, dependent on parser output & structure
//! probably needs to go into the parser, not used in my code
// Static method to check if a given path is a CGI request
// Returns true if the path contains "/cgi-bin/", ".py", or ".cgi"
bool CGI::isCGIRequest(const std::string& path) {
    size_t len = path.length();
    return (len > 3 && path.substr(len - 3) == ".py") ||
           (len > 4 && path.substr(len - 4) == ".php");
}

std::string CGI::resolveCGIPath(const std::string& uri) {
    char buffer[PATH_MAX];
    getcwd(buffer, PATH_MAX);
    std::string projectRoot = std::string(buffer);
    
    // Debug print
    std::cout << "Project root: " << projectRoot << std::endl;
    std::cout << "URI: " << uri << std::endl;
    
    // Remove the leading "/cgi-bin" from uri and combine with project root
    std::string relativePath = uri.substr(8); // remove "/cgi-bin"
    std::string fullPath = projectRoot + "/cgi-bin" + relativePath;
    
    // Debug print
    std::cout << "Full resolved path: " << fullPath << std::endl;
    
    return fullPath;
}

// Main method to handle a CGI request
// it was determined before if CGI request or not
// Sets up the environment, executes the CGI script and sends the response
void CGI::handleCGIRequest(HttpRequest request) {
    std::cerr << "1. Starting handleCGIRequest..." << std::endl;
    std::cerr << "Request URI: " << request.uri << std::endl;
    std::cerr.flush();
    
    try {
        // Resolve the actual path to the script
        std::string fullScriptPath = resolveCGIPath(request.uri);
        std::cerr << "2. Resolved path: " << fullScriptPath << std::endl;
        
        std::cerr << "File exists? " << (access(fullScriptPath.c_str(), F_OK) == 0 ? "Yes" : "No") << std::endl;
        std::cerr << "File executable? " << (access(fullScriptPath.c_str(), X_OK) == 0 ? "Yes" : "No") << std::endl;
        
        // Update the scriptPath member variable
        scriptPath = fullScriptPath;

        if (access(fullScriptPath.c_str(), F_OK | X_OK) == -1) {
            std::cerr << "Error: " << strerror(errno) << std::endl;
            throw std::runtime_error("Script not found or not executable: " + fullScriptPath);
        }
        
        std::cerr << "3. Setting environment..." << std::endl;
        std::cerr.flush();
        setCGIEnvironment(request);
        
        std::cerr << "4. About to execute CGI..." << std::endl;
        std::cerr.flush();
        std::string response = executeCGI();
        
        std::cerr << "5. Sending response..." << std::endl;
        std::cerr.flush();
        sendResponse(response);
        
        std::cerr << "6. CGI request completed" << std::endl;
        std::cerr.flush();
        return; // Make sure we return after sending the response
    }
    catch (const std::exception& e) {
        std::cerr << "Error in handleCGIRequest: " << e.what() << std::endl;
        std::cerr.flush();
        // Send error response to client
        std::string errorResponse = "Status: 500\r\nContent-Type: text/plain\r\n\r\nCGI Error: " + std::string(e.what());
        sendResponse(errorResponse);
        throw; // Re-throw to let the server handle the connection cleanup
    }
}

// Sets up the environment variables for the CGI script
void CGI::setCGIEnvironment(const HttpRequest& httpRequest) const
{
    setenv("REQUEST_METHOD", httpRequest.method.c_str(), 1);
    setenv("QUERY_STRING", httpRequest.queryString.c_str(), 1);
    setenv("CONTENT_LENGTH", std::to_string(httpRequest.body.length()).c_str(), 1);
    setenv("SCRIPT_NAME", httpRequest.uri.c_str(), 1);
    setenv("SERVER_PROTOCOL", httpRequest.version.c_str(), 1);
    setenv("CONTENT_TYPE", httpRequest.contentType.c_str(), 1);

    // for all other env variables
    std::map<std::string, std::string>::const_iterator it;
    for (it = httpRequest.headers.begin(); it != httpRequest.headers.end(); ++it) {
        std::string envName = "HTTP_" + it->first;
        std::replace(envName.begin(), envName.end(), '-', '_');
        std::transform(envName.begin(), envName.end(), envName.begin(), ::toupper);
        setenv(envName.c_str(), it->second.c_str(), 1);
    }
}

// Executes the CGI script and handles its output
std::string CGI::executeCGI() {
    std::cerr << "A. Starting executeCGI..." << std::endl;
    std::cerr.flush();

    // Create pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        throw std::runtime_error("Pipe creation failed");
    }

    std::cerr << "B. Pipe created, forking..." << std::endl;
    std::cerr.flush();
    
    pid_t pid = fork();
    std::cerr << "C. Fork result: " << pid << std::endl;
    std::cerr.flush();

    if (pid == -1) {
        throw std::runtime_error("Fork failed");
    }

    if (pid == 0) {  // Child process
        std::cerr << "D. Child process starting..." << std::endl;
        std::cerr << "Script path: " << scriptPath << std::endl;
        std::cerr.flush();
        
        close(pipefd[0]);  // Close read end
        
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            std::cerr << "dup2 failed: " << strerror(errno) << std::endl;
            exit(1);
        }
        
        close(pipefd[1]);

        // this is my specific python path (cmd: which python3)
        const char* pythonPath = "/Library/Frameworks/Python.framework/Versions/3.11/bin/python3";
        char *const args[] = {
            const_cast<char*>(pythonPath),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };

        std::cerr << "About to execve with:" << std::endl;
        std::cerr << "Python path: " << pythonPath << std::endl;
        std::cerr << "Script path: " << scriptPath << std::endl;
        std::cerr.flush();

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
        int ready = select(pipefd[0] + 1, &readfds, NULL, NULL, &tv);
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

    std::cout << "CGI Response:\n" << response << std::endl;
    std::cout << "=== CGI Execution End ===" << std::endl;
    
    return response;
}

//! not used anymore it's now directly integrated in exectuteCGI, let's see if i want to keep it after response flow is clear

// Reads the CGI script's output from the pipe
// this is where the response is captured
std::string CGI::readFromPipe(int pipefd) const
{
    std::stringstream return_string;
    char buffer[1024];
    int bytesRead;

    while ((bytesRead = read(pipefd, buffer, sizeof(buffer))) > 0) {
        return_string.write(buffer, bytesRead);
    }

    close(pipefd);
    return return_string.str();
}

//! use carinas functions instead!!!
// Sends the complete response (headers + body) to the client
void CGI::sendResponse(const std::string& response) const
{
    std::string fullResponse = response;
    
    // Check if the CGI script provided headers
    if (fullResponse.find("Status: ") == std::string::npos &&
        fullResponse.find("Content-Type: ") == std::string::npos) {
        // If no headers, add default headers
        fullResponse = "Status: 200 OK\r\nContent-Type: text/html\r\n\r\n" + fullResponse;
    } else if (fullResponse.find("\r\n\r\n") == std::string::npos) {
        // If headers are present but no empty line, add it
        fullResponse += "\r\n";
    }

    // Send the full response to the client
    // Log errors if sending the response fails.
     if (send(clientSocket, fullResponse.c_str(), fullResponse.length(), 0) == -1) {
        perror("send");
    }
}
