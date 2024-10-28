#include "cgi.hpp"
#include "HttpRequest.hpp"

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

CGI::CGI() {}

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
    return path.find("/cgi-bin/") != std::string::npos || 
           path.find(".py") != std::string::npos || 
           path.find(".cgi") != std::string::npos;
}

// Main method to handle a CGI request
// it was determined before if CGI request or not
// Sets up the environment, executes the CGI script and sends the response
void CGI::handleCGIRequest(HttpRequest httpRequest)
{
    setCGIEnvironment(httpRequest);
    /*
        To understand what will be returned if execve is successfully called:
        - response will not be directly set by the execve call, since it never returns
        - instead executeCGI() function forks the current process before calling execve
        - The parent process waits for the child to complete and captures its output.
        - this output is then returned by executeCGI() and set to response
        ->>> response string contains the output of the CGI script, not the return value of execve itself
    */
    std::string response = executeCGI(); // return value of readFromPipe function
    sendResponse(response);
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

    // Set additional headers as environment variables if needed
    for (const auto& header : httpRequest.headers) {
        std::string envName = "HTTP_" + header.first;
        std::replace(envName.begin(), envName.end(), '-', '_'); // Replace '-' with '_'
        std::transform(envName.begin(), envName.end(), envName.begin(), ::toupper); // Convert to uppercase
        setenv(envName.c_str(), header.second.c_str(), 1);
    }
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
        // allows the parent process to capture the output of the CGI script
        dup2(pipefd[1], STDOUT_FILENO);

        // If it's a POST request, redirect the stdin to handle the request body
        if (method == "POST" && !requestBody.empty()) {
            int bodyPipe[2];
            if (pipe(bodyPipe) == -1) {
                perror("pipe");
                exit(1);
            }
            write(bodyPipe[1], requestBody.c_str(), requestBody.size());
            close(bodyPipe[1]);
            dup2(bodyPipe[0], STDIN_FILENO);
            close(bodyPipe[0]);
        }
        //! do post requests always have a body?
        //! probably different approach for post requests needed!
        // do i need the case below????
       /*  if (!requestBody.empty()) {
            dup2(pipefd[1], STDIN_FILENO); //! makes no sense? should probably redirect to fd that provides the request body?
        } */

        // Execute the CGI script using exec (replace the child process with the CGI script)
        /*
            args array to hold the arguments for the CGI script;
            first argument: script path, second argument: NULL (indicating the end of the argument list)
            const cast: execve expects non-const array
            c_str: because execve is a c function and expects a c-style string
        */
        char *const args[] = {const_cast<char*>(scriptPath.c_str()), NULL};
        /* 
            NULL would normally be envs but here just means use the current environment, meaning
            all variables set by setCGIEnvironment()
        */
        execve(scriptPath.c_str(), args, NULL); //! use environ instead of NULL??? // are variables actually passed here, since they are only set with setenv

        // If execve fails, exit the child process (if successful, execve never returns)
        perror("execve");
        exit(1);
    } else if (pid > 0) {  // Parent process (captures output) / do I need this condition or could I just use else????
        close(pipefd[1]);  // Close the writing end in the parent

        // Wait for the child process (CGI script) to complete
        waitpid(pid, NULL, 0);

        return readFromPipe(pipefd[0]);
    }
    return "Status: 500 Internal Server Error\r\n\r\nCGI execution failed";
}

// Reads the CGI script's output from the pipe
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
