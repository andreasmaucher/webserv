#include "../../include/responseHandler.hpp"
#include "../../include/cgi.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/httpResponse.hpp"
#include "../../include/debug.hpp"

void ResponseHandler::finalizeCGIErrorResponse(int &fd, HttpRequest &request, HttpResponse &response) {
    request.complete = true;
    response.complete = true;
    ResponseHandler::responseBuilder(response);
    WebService::setPollfdEventsToOut(fd);
}

void ResponseHandler::prepareCGIErrorResponse(HttpResponse &response, 
    int status_code, const std::string &reason, 
    const std::string &body, const std::string &allowed_methods) {
    
    response.status_code = status_code;
    response.reason_phrase = reason;
    // Convert status_code to string using ostringstream
    std::ostringstream ss;
    ss << status_code;
    
    // Read the error page HTML file
    std::string error_page_path = "www/errors/" + ss.str() + ".html";
    std::ifstream error_file(error_page_path.c_str());
    
    if (error_file.is_open()) {
        // Read HTML content
        std::stringstream buffer;
        buffer << error_file.rdbuf();
        response.body = buffer.str();
        response.setHeader("Content-Type", "text/html");
        error_file.close();
    } else {
        // Fallback to plain text if HTML file not found
        response.body = body;
        response.setHeader("Content-Type", "text/plain");
    }

    response.setHeader("Connection", "close");
    
    if (!allowed_methods.empty()) {
        response.setHeader("Allow", allowed_methods);
    }
    
    response.close_connection = true;
}

bool ResponseHandler::handleCGIErrors(int &fd, Server &config, HttpRequest &request, HttpResponse &response) {

  (void)config;
   // if it is not a CGI request, continue normal processing
   std::cout << "request.uri: " << request.uri << std::endl;
   if (request.uri.find("/cgi-bin") != 0) {
        std::cout << "NOT CGI REQUEST??" << std::endl;
        return true;
    }
    // Method validation
    if (request.method != "GET" && request.method != "POST" && request.method != "DELETE") {
        std::cout << "within CGI error check 2" << std::endl;
        prepareCGIErrorResponse(response, 405, "Method Not Allowed", 
            "Method Not Allowed for CGI requests", "GET, POST, DELETE");
        finalizeCGIErrorResponse(fd, request, response);
        return false;
    }
    if (CGI::isCGIRequest(request.uri)) {
        response.is_cgi_response = true; // used to differentiate between cgi and static error pages
    }
    // Check for valid CGI file extension first (sends error whenever the file is not a .py)
    if (!CGI::isCGIRequest(request.uri)) {
      std::cout << "within CGI error check 1" << std::endl;
        prepareCGIErrorResponse(response, 400, "Bad Request", 
          "Bad Request: Invalid CGI file type. Only .py files are allowed.", "");
        finalizeCGIErrorResponse(fd, request, response);
        return false;
    }
    // Check if script exists
    try {
        CGI::resolveCGIPath(request.uri);
    } catch (const std::runtime_error& e) {
        prepareCGIErrorResponse(response, 404, "Not Found", 
            std::string("Script not found: ") + e.what(), "");
        finalizeCGIErrorResponse(fd, request, response);
        return false;
    }
    // Check if there's a path parameter (file reference)
    std::string pathInfo;
    //! this gets wrongly triggered by delete requests!
    /* try { 
        pathInfo = CGI::extractPathInfo(request.uri);
    } 
    catch (const std::runtime_error& e) {
        prepareCGIErrorResponse(response, 400, "Bad Request", 
        std::string("Invalid file extension or other PATH_INFO error: ") + e.what(), "");
        finalizeCGIErrorResponse(fd, request, response);
        return false; 
    } */
    if (!pathInfo.empty())
    {
        // Check if the referenced file exists in uploads directory
        std::string filePath = "www/uploads/" + pathInfo;
        if (access(filePath.c_str(), F_OK) == -1)
        {
            prepareCGIErrorResponse(response, 404, "Not Found", 
            std::string("Referenced file not found: ") + pathInfo, "");
            finalizeCGIErrorResponse(fd, request, response);
            return false;
        }
    }
    return true; // No errors, continue with normal processing
}

void ResponseHandler::processRequest(int &fd, Server &config, HttpRequest &request, HttpResponse &response)
{
  //! All CGI error checks need to happen in here before handleCGIRequest
  // if (!handleCGIErrors(fd, config, request, response)) {
  //       return;
  //   }
  // // Find matching route
  // if (!findMatchingRoute(config, request, response))
  // {
  //   DEBUG_MSG("Route status", "No matching route found");
  //   response.status_code = 404;
  //   return;
  // }
  
  // const Route *route = request.route; // Route is already stored in request
 
  // DEBUG_MSG("Route found", route->uri);
  // DEBUG_MSG("Is CGI route", (route->is_cgi ? "yes" : "no"));
  // //  If it's a CGI request, handle it
  //? CGI RESPONSE request process starts here
  // if (route->is_cgi)
  // {
  //   DEBUG_MSG_1("Request status", "Handling CGI request");
  //   try
  //   {
  //     CGI cgi;
  //     cgi.handleCGIRequest(fd, request, response);
  //     // Always start with HTTP status line for valid HTTP/1.1 response
  //     //! UNDO
  //     /* response.status_code = 200;
  //     response.version = "HTTP/1.1";
  //     response.reason_phrase = "OK"; */

  //     // Parse CGI headers into response object
  //     size_t headerEnd = request.body.find("\r\n\r\n");
  //     if (headerEnd != std::string::npos)
  //     {
  //       std::string headers = request.body.substr(0, headerEnd);
  //       // Set body to everything AFTER the headers and the blank line
  //       response.body = request.body.substr(headerEnd + 4);

  //       // Parse headers
  //       size_t pos = 0;
  //       std::string headerSection = headers;
  //       while ((pos = headerSection.find("\r\n")) != std::string::npos)
  //       {
  //         std::string header = headerSection.substr(0, pos);
  //         size_t colonPos = header.find(": ");
  //         if (colonPos != std::string::npos)
  //         {
  //           std::string name = header.substr(0, colonPos);
  //           std::string value = header.substr(colonPos + 2);
  //           response.setHeader(name, value);
  //         }
  //         headerSection = headerSection.substr(pos + 2);
  //       }

  //       // Handle last header if exists
  //       if (!headerSection.empty())
  //       {
  //         size_t colonPos = headerSection.find(": ");
  //         if (colonPos != std::string::npos)
  //         {
  //           std::string name = headerSection.substr(0, colonPos);
  //           std::string value = headerSection.substr(colonPos + 2);
  //           response.setHeader(name, value);
  //         }
  //       }
  //     }
  //     response.close_connection = true;
  //     DEBUG_MSG("ResponseHandler::processRequest response.close_connection = true", response.close_connection);
  //     request.complete = true;
  //     return;
  //   }
  //   catch (const std::exception &e)
  //   {
  //     DEBUG_MSG("CGI execution failed", e.what());
  //     response.status_code = 500;
  //     response.body = "CGI execution failed";
  //     response.close_connection = true;
  //     request.complete = true; // Make sure request is marked complete
  //     return;
  //   }
  // }

  /* -- Carinas part starts here -- */
  DEBUG_MSG("Status", "Processing request");
  //  from here on, we will populate & use the response object status code only
  response.status_code = request.error_code; //do at the end in populateResponse or responseBuilder
  //  find connection header and set close_connection in response object
  if (request.error_code == 0)
  { // Check error_code but don't set response status
    ResponseHandler::routeRequest(fd, config, request, response);
  }
  // if (response.status_code == 0)
  // {
  //   // serve_file, process_api_request & populate response body (content) or error code
  //   // ResponseHandler::routeRequest(config, request, response);
  //   response.status_code = 200;
  // }
  //! REDIRECTS
  if (!request.route->redirect_uri.empty())
  {
    std::cout << "redirecting to: aaAAAAAA " << request.route->redirect_uri << std::endl;
    response.status_code = 301;
    response.setHeader("Location", request.route->redirect_uri);
    response.reason_phrase = "Moved Permanently";
    response.setHeader("Content-Length", "0");
  }
  /* // if redirect_uri is set, set the redirect exit code and return true
  std::cout << "best_match->redirect_uri: " << best_match->redirect_uri << std::endl;
  if (!best_match->redirect_uri.empty())
  {
    DEBUG_MSG("Status", "Redirect found to: " + best_match->redirect_uri);
    std::cout << "redirecting to: " << best_match->redirect_uri << std::endl;
    response.status_code = 301;
    //response.setRedirect(best_match->redirect_uri);
    std::cout << "response.status_code for redirect: " << response.status_code << std::endl;
    return true;
  } */

  if (request.headers.find("Connection") != request.headers.end() && request.headers["Connection"] == "close")
    response.close_connection = true;
  // fill the rest of the response fields to create the final response
  // the ones with error code from parser go directly here
  // to do: (check if in the parser I set some other value like headers etc since here I'm not passing the request object)
  ResponseHandler::responseBuilder(response);
}

void ResponseHandler::routeRequest(int &fd, Server &config, HttpRequest &request, HttpResponse &response)
{
  DEBUG_MSG("Status", "Routing request");
  MimeTypeMapper mapper;
  // Find matching route in server, verify the requested method is allowed in that route and if the requested type content is allowed
  if (findMatchingRoute(config, request, response) && isMethodAllowed(request, response) && mapper.isContentTypeAllowed(request, response))
  {
    if (request.route->is_cgi)
    { // at this point its been routed already and checked if (CGI)extension is allowed
      // DEBUG_MSG("Status", "Calling CGI handler");
      //! CGI PROCESS STARTS
      // CGI cgiHandler;
      // cgiHandler.handleCGIRequest(fd, request, response);
      DEBUG_MSG_1("Request status", "Handling CGI request");
      try
      {
        CGI cgi;
        cgi.handleCGIRequest(fd, request, response);
        // Always start with HTTP status line for valid HTTP/1.1 response
        //! UNDO
        /* response.status_code = 200;
        response.version = "HTTP/1.1";
        response.reason_phrase = "OK"; */

        // Parse CGI headers into response object
        size_t headerEnd = request.body.find("\r\n\r\n");
        if (headerEnd != std::string::npos)
        {
          std::string headers = request.body.substr(0, headerEnd);
          // Set body to everything AFTER the headers and the blank line
          response.body = request.body.substr(headerEnd + 4);

          // Parse headers
          size_t pos = 0;
          std::string headerSection = headers;
          while ((pos = headerSection.find("\r\n")) != std::string::npos)
          {
            std::string header = headerSection.substr(0, pos);
            size_t colonPos = header.find(": ");
            if (colonPos != std::string::npos)
            {
              std::string name = header.substr(0, colonPos);
              std::string value = header.substr(colonPos + 2);
              response.setHeader(name, value);
            }
            headerSection = headerSection.substr(pos + 2);
          }

          // Handle last header if exists
          if (!headerSection.empty())
          {
            size_t colonPos = headerSection.find(": ");
            if (colonPos != std::string::npos)
            {
              std::string name = headerSection.substr(0, colonPos);
              std::string value = headerSection.substr(colonPos + 2);
              response.setHeader(name, value);
            }
          }
        }
        response.close_connection = true;
        DEBUG_MSG("ResponseHandler::processRequest response.close_connection = true", response.close_connection);
        request.complete = true;
        return;
      }
      catch (const std::exception &e)
      {
        DEBUG_MSG("CGI execution failed", e.what());
        response.status_code = 500;
        response.body = "CGI execution failed";
        response.close_connection = true;
        request.complete = true; // Make sure request is marked complete
        return;
      }
    }
    else
    {
      DEBUG_MSG("Status", "Calling static content handler");
      staticContentHandler(request, response);
    }
  }
}

void ResponseHandler::staticContentHandler(HttpRequest &request, HttpResponse &response)
{
  if (request.method == "GET")
  {
    ResponseHandler::serveStaticFile(request, response);
  }
  else if (request.method == "POST")
  {
    ResponseHandler::processFileUpload(request, response);
  }
  else if (request.method == "DELETE")
  {
    ResponseHandler::processFileDeletion(request, response);
  }
  else
  {
    response.status_code = 405;
  }
}

// creates a directory listing html page if autoindex is 'on'
void ResponseHandler::generateDirectoryListing(const HttpRequest &request, HttpResponse &response)
{
  std::string html = "<html>\n<head>\n"
                     "<title>Index of " +
                     request.uri + "</title>\n"
                                   "<style>\n"
                                   "body { font-family: Arial, sans-serif; margin: 40px; }\n"
                                   "h1 { color: #333; }\n"
                                   "a { text-decoration: none; color: #0066cc; }\n"
                                   "a:hover { text-decoration: underline; }\n"
                                   "</style>\n"
                                   "</head>\n"
                                   "<body>\n"
                                   "<h1>Index of " +
                     request.uri + "</h1>\n"
                                   "<hr>\n"
                                   "<pre>\n";

  DIR *dir = opendir(request.path.c_str());
  if (!dir)
  {
    response.status_code = 500;
    response.reason_phrase = "Internal Server Error";
    return;
  }

  // Add parent directory link if not at root
  if (request.uri != "/")
  {
    html += "<a href=\"../\">../</a>\n";
  }

  struct dirent *entry;
  while ((entry = readdir(dir)))
  {
    std::string name = entry->d_name;

    // Skip . and ..
    if (name == "." || name == "..")
    {
      continue;
    }

    // Get file info
    struct stat file_stat;
    std::string full_path = request.path + "/" + name;
    if (stat(full_path.c_str(), &file_stat) == 0)
    {
      // Convert size to string using stringstream
      std::ostringstream ss;
      ss << file_stat.st_size << " bytes";
      std::string size = ss.str();

      // Format name with trailing slash for directories
      std::string display_name = name;
      if (S_ISDIR(file_stat.st_mode))
      {
        display_name += "/";
      }

      // Add entry (name padded to 50 chars, then size)
      html += "<a href=\"" + display_name + "\">" + display_name + "</a>";
      html += std::string(50 - display_name.length(), ' ') + size + "\n";
    }
  }
  closedir(dir);

  html += "</pre>\n<hr>\n</body>\n</html>";

  response.body = html;
  response.status_code = 200;
  response.reason_phrase = "OK";
  response.setHeader("Content-Type", "text/html");

  // Convert content length to string
  std::ostringstream length_ss;
  length_ss << html.length();
  response.setHeader("Content-Length", length_ss.str());
}

// at this point the path is set in the request!
void ResponseHandler::serveStaticFile(HttpRequest &request, HttpResponse &response)
{
  DEBUG_MSG("Status", "Serving static file");
  // handle directory listing first, since the next block sets response error codes
  //  Default Index File: If directory listing is off, the server typically looks for a default index file, like index.html or index.php, in the requested directory. If found, it serves that file. If not found, it may return a 403 Forbidden or 404 Not Found response, depending on server configuration.
  //  Directory Listing Enabled: If directory listing is enabled, and no default index file exists, the server will dynamically generate a page showing the contents of the directory, so the user can browse the files.
  //! ANDY: setting is_directory to false here makes no sense and breaks autoindex. needs new logic
  /* if (request.is_directory)
  {
    request.file_name = DEFAULT_FILE; // index of all server directories
    request.is_directory = false;     // to avoid the fileExists directory check //! DIRECTORY WE CAN NOT DO THIS?!
    // if (config.routes[request.uri].directory_listing) {
    //   DEBUG_MSG("Implement directory listing" << std::endl;
    //   //search for & serve default index file or generate dynamically
    //   // Generate directory listing
    // }
    // else {
    //   //search for & serve default index file or return error
    //   response.status_code = 403;  // Forbidden if no file is found and listing is off or Not Found
    // }
  } */

  //! DIRECTORY
  /* if (request.is_directory) {
    std::cout << "removing index.html from path for directory checks" << std::endl;
    // Remove index.html from path for directory checks
    size_t pos = request.path.find("/index.html");
    if (pos != std::string::npos) {
        request.path = request.path.substr(0, pos);
    }
    // Remove double slashes
    while (request.path.find("//") != std::string::npos) {
        request.path.replace(request.path.find("//"), 2, "/");
    }
  } */
  ResponseHandler::setFullPath(request);

  // Move directory redirect check here, before any file operations
  if (request.is_directory && !request.uri.empty() && request.uri[request.uri.length() - 1] != '/') {
      response.status_code = 301;
      response.reason_phrase = "Moved Permanently";
      response.setHeader("Location", request.uri + "/");
      response.setHeader("Content-Length", "0");
      response.setHeader("Connection", "close");
      response.body = "";
      response.complete = true;
      response.close_connection = true;
      return;
  }
  
  if (request.is_directory)
  {
    // autoindex in nginx is by default disabled for POST and DELETE
    if (request.method == "POST" || request.method == "DELETE")
    {
      response.status_code = 405; // Method Not Allowed
      response.reason_phrase = "Method Not Allowed";
      response.setHeader("Allow", "GET"); // Indicate which methods are allowed
      return;
    }

    // Try index.html first
    std::string original_path = request.path;
    request.file_name = "index.html";
    ResponseHandler::setFullPath(request);

    if (fileExists(request, response) && hasReadPermission(request.path, response))
    {
      readFile(request, response);
      return;
    }

    // Restore path for directory listing
    request.path = original_path;
    request.is_directory = true;

    if (request.route->autoindex)
    {
      generateDirectoryListing(request, response);
    }
    else
    {
      response.status_code = 403; // Directory listing disabled
    }
    return;
  }

  // Regular file handling
  ResponseHandler::setFullPath(request);
  if (ResponseHandler::fileExists(request, response) &&
      ResponseHandler::hasReadPermission(request.path, response))
  {
    readFile(request, response);
  }
}

bool ResponseHandler::readFile(HttpRequest &request, HttpResponse &response)
{
  std::ifstream file;
  errno = 0;
  file.open(request.path.c_str(), std::ios::in | std::ios::binary);

  if (!file.is_open())
  {
    DEBUG_MSG_1("Cant open file", strerror(errno));
    response.status_code = 500;
    return false;
  }
  if (file.fail() || file.bad())
  {
    DEBUG_MSG_1("Error", "Failed to read file: " + request.path);
    DEBUG_MSG_1("Error details", strerror(errno));
    file.close();
    return false;
  }
  errno = 0;

  std::ostringstream buffer;
  buffer << file.rdbuf();
  if (file.fail() || file.bad())
  {
    DEBUG_MSG_1("Error", "Failed while reading file: " + request.path);
    DEBUG_MSG_1("Error details", strerror(errno));
    file.close();
    return false;
  }
  response.body = buffer.str();
  file.close();
  response.status_code = 200;
  response.setHeader("Content-Type", request.content_type);

  return true;
}

void ResponseHandler::processFileUpload(HttpRequest &request, HttpResponse &response)
{
  DEBUG_MSG("Status", "Processing file upload");

  if (request.body.empty())
  {
    response.status_code = 400;
    return;
  }
  constructFullPath(request, response);

  if (!fileExists(request, response))
  {
    writeToFile(request, response);
  }
}

void ResponseHandler::writeToFile(HttpRequest &request, HttpResponse &response)
{
  // Open the file and write request body into it
  // Permission check??
  std::ofstream file(request.path.c_str(), std::ios::binary);
  if (file.is_open())
  {
    file << request.body;
    file.close();
    response.status_code = 201;
    response.body = "File uploaded successfully";
    response.setHeader("Content-Type", "text/plain");
    DEBUG_MSG("Upload status", "File uploaded successfully");
  }
  else
  {
    DEBUG_MSG("Error", "Failed writing to file");
    response.status_code = 500;
  }
}

void ResponseHandler::processFileDeletion(HttpRequest &request, HttpResponse &response)
{
  DEBUG_MSG("Status", "Processing file deletion");
  constructFullPath(request, response);

  // Check if the file exists at the location. Permission check??
  if (fileExists(request, response))
  {
    DEBUG_MSG("Status", "Deleting file");
    removeFile(request, response);
  }
}

void ResponseHandler::removeFile(HttpRequest &request, HttpResponse &response)
{
  if (std::remove(request.path.c_str()) == 0)
  {
    response.status_code = 200;
  }
  else
  {
    response.status_code = 500;
  }
}

//--------------------------------------------------------------------------

// HELPER FUNCTIONS

// make sure the path is sanitized to avoid security breaches!
// stat system call return 0 if file is accessible
bool ResponseHandler::fileExists(HttpRequest &request, HttpResponse &response)
{
  struct stat path_stat;

  // // Sanitize path to avoid traversal issues (implement a sanitization function as needed)
  // if (!sanitizePath(request.path)) {
  //     response.status_code = 400; // Bad Request or similar for unsafe path
  //     return false;
  // }
  // if (access(request.path.c_str(), F_OK) == 0) {
  /*   if (stat(request.path.c_str(), &path_stat) == 0 && (!request.is_directory && S_ISREG(path_stat.st_mode)))
    {
      DEBUG_MSG("File status", "File exists and accessible");
      if (request.method == "POST")
      {
        DEBUG_MSG("File status", "File already exists");
        response.status_code = 409;
      }
      return true;
    } */

  // std::cout << "Working directory: " << getcwd(NULL, 0) << std::endl;
  //! DIRECTORY
  // First check if path exists
  if (stat(request.path.c_str(), &path_stat) == 0)
  {
    // If it's a directory and GET request with autoindex, allow it
    if (request.is_directory && request.method == "GET" && request.route->autoindex)
    {
      return true;
    }

    // Original check for regular files
    if (!request.is_directory && S_ISREG(path_stat.st_mode))
    {
      DEBUG_MSG("File status", "File exists and accessible");
      if (request.method == "POST")
      {
        DEBUG_MSG("File status", "File already exists");
        response.status_code = 409;
      }
      return true;
    }
  }
  if (request.method == "GET" || (request.method == "DELETE" && !request.is_directory))
  {
    DEBUG_MSG("File status", "File does not exist");
    response.status_code = 404;
  }
  else if (request.method == "DELETE" && request.is_directory)
  {
    DEBUG_MSG("Status", "Directory deletion not implemented");
    response.status_code = 501;
  }
  DEBUG_MSG("File status", "File does not exist or not accessible");
  return false;
}

// // simple sanitization function (not exhaustive)
// bool ResponseHandler::sanitizePath(const std::string &path) {
//     return (path.find("..") == std::string::npos); // Example check to block directory traversal
// }

void ResponseHandler::constructFullPath(HttpRequest &request, HttpResponse &response)
{
  // path is relative to the current working directory
  //(where the server is running, if we execute from inside src for ex ../werbserv, the root dir is src and it won't work)
  // std::string full_path = request.route->path; // Assuming route_path is the matched route's path
  request.path = request.route->path;
  std::cout << "request.path in constructFullPath: " << request.path << std::endl;

  // Check if file_name contains subdirectories (e.g., "subdir/filename.txt")
  size_t last_slash_pos = request.file_name.find_last_of('/');
  if (!handleSubdirectory(request, response, last_slash_pos))
    return;

  finalizeFullPath(request, last_slash_pos);
}

void ResponseHandler::finalizeFullPath(HttpRequest &request, size_t &last_slash_pos)
{
  // Append the filename (without subdirectory) to the full path
  if (last_slash_pos != std::string::npos)
  {
    request.file_name = request.file_name.substr(last_slash_pos + 1);
    DEBUG_MSG("File name", request.file_name);
  }
  // If no filename, extract or generate filename. Only for POST
  else if (request.method == "POST")
  {
    extractOrGenerateFilename(request);
  }

  request.path += "/" + request.file_name;
  std::cout << "request.path in finalizeFullPath: " << request.path << std::endl;
  DEBUG_MSG("Full path to content", request.path);

  return;
}

bool ResponseHandler::handleSubdirectory(HttpRequest &request, HttpResponse &response, size_t &last_slash_pos)
{
  // Check if file_name contains subdirectories (e.g., "subdir/filename.txt")
  if (last_slash_pos != std::string::npos)
  {
    // Extract subdirectory path (e.g., "subdir") and update full_path
    std::string sub_dir = request.file_name.substr(0, last_slash_pos);
    DEBUG_MSG("Subdirectory found", sub_dir);
    request.path += "/" + sub_dir;

    // Check if the directory path exists up to this point
    struct stat path_stat;
    if (stat(request.path.c_str(), &path_stat) != 0 || !S_ISDIR(path_stat.st_mode))
    {
      DEBUG_MSG("Path status", "Subdirectory/path does not exist");
      response.status_code = 404;
      return false;
    }
  }
  return true;
}

std::string ResponseHandler::sanitizeFileName(std::string &file_name)
{
  std::string sanitized;
  for (size_t i = 0; i < file_name.size(); ++i)
  {
    char c = file_name[i];
    if (std::isalnum(c) || c == '_' || c == '-' || (c == '.' && i > 0 && i < file_name.size() - 1))
    {
      sanitized += c;
    }
  }

  // Limit length if needed, e.g., 100 characters
  if (sanitized.length() > 100)
  {
    sanitized = sanitized.substr(0, 100);
  }

  return sanitized;
}

// Helper function to extract the filename from headers or generate one
void ResponseHandler::extractOrGenerateFilename(HttpRequest &request)
{
  std::map<std::string, std::string>::const_iterator it = request.headers.find("Content-Disposition");
  if (it != request.headers.end())
  {
    // assuming format: Content-Disposition: attachment; filename="myfile.txt"
    size_t pos = it->second.find("filename=");
    if (pos != std::string::npos)
    {
      request.file_name = it->second.substr(pos + 9); // Skip "filename="
      std::string no_quotes;
      for (size_t i = 0; i < request.file_name.size(); ++i)
      {
        if (request.file_name[i] != '\"')
        {
          no_quotes += request.file_name[i];
        }
      }
      request.file_name = no_quotes; // Replace original with no_quotes version
      request.file_name = ResponseHandler::sanitizeFileName(request.file_name);
      DEBUG_MSG("Extracted file name", request.file_name);
    }
  }
  if (request.file_name.empty())
  {
    request.file_name = generateTimestampName();
    DEBUG_MSG("Generated file name", request.file_name);
  }
}

std::string ResponseHandler::generateTimestampName()
{
  std::time_t current_time = std::time(0);

  std::ostringstream oss;
  oss << current_time;

  std::string file_name = "upload_" + oss.str();

  return file_name;
}

bool ResponseHandler::hasReadPermission(const std::string &file_path, HttpResponse &response)
{
  if (access(file_path.c_str(), R_OK) == 0)
    return true;
  else
  {
    response.status_code = 403;
    return false;
  }
}

// Store the best match if there are multiple matches (longest prefix match)
bool ResponseHandler::findMatchingRoute(Server &server, HttpRequest &request, HttpResponse &response)
{
  DEBUG_MSG("Status", "Finding matching route for [" + request.uri + "]");
  const std::map<std::string, Route> &routes = server.getRoutes();
  const Route *best_match = NULL;
  size_t longest_match_length = 0;

  // DEBUG output for all routes
  DEBUG_MSG("Status", "Available routes:");
  for (std::map<std::string, Route>::const_iterator it = routes.begin(); it != routes.end(); ++it)
  {
    DEBUG_MSG("Status", "Route: [" + it->first + "] CGI: " + (it->second.is_cgi ? "Yes" : "No"));
  }

  for (std::map<std::string, Route>::const_iterator it = routes.begin(); it != routes.end(); ++it)
  {
    const std::string &route_uri = it->first;
    DEBUG_MSG("Status", "Comparing request [" + request.uri + "] to route [" + route_uri + "]");
    const Route &route_object = it->second;

    bool is_match = false;
    if (route_object.is_cgi)
    {
      // For CGI routes, just check if the URI starts with the route path
      std::cout << "request.uri: " << request.uri << std::endl;
      std::cout << "route_uri: " << route_uri << std::endl;
      is_match = (request.uri.compare(0, 9, "/cgi-bin/") == 0);
      std::cout << "is_match: " << is_match << std::endl;
    }
    else
    {
      // For non-CGI routes, check for exact match or proper path separation
      is_match = (request.uri.compare(0, route_uri.size(), route_uri) == 0);
      if (is_match && request.uri.size() > route_uri.size())
      {
        char next_char = request.uri[route_uri.size()];
        is_match = (next_char == '/' || route_uri[route_uri.size() - 1] == '/');
      }
    }

    if (is_match)
    {
      if (route_object.is_cgi)
      {
        best_match = &route_object;
        break;
      }
      size_t match_length = route_uri.size();
      if (match_length > longest_match_length)
      {
        best_match = &route_object;
        longest_match_length = match_length;
        DEBUG_MSG("Status", "Found better match: [" + route_uri + "]");
      }
    }
  }

  if (best_match == NULL)
  {
    DEBUG_MSG("Status", "No matching route found");
    response.status_code = 404;
    return false;
  }

  DEBUG_MSG("Status", "Best matching route: [" + best_match->uri + "] CGI: " + (best_match->is_cgi ? "Yes" : "No"));
  request.route = best_match;
  std::cout << "matching route: " << request.route->uri << std::endl;
  // request.is_cgi = best_match->is_cgi;
  return true;
}

bool ResponseHandler::isMethodAllowed(const HttpRequest &request, HttpResponse &response)
{
  DEBUG_MSG("Status", "Checking if method " + request.method + " is allowed");
  //  Verify the requested method is allowed in that route searching in the set
  if (request.route->methods.find(request.method) == request.route->methods.end())
  {
    DEBUG_MSG("Status", "Method not allowed in route");
    response.status_code = 405;
    std::string header_key = "Allow";
    std::string header_value = ResponseHandler::createAllowedMethodsStr(request.route->methods);
    response.setHeader(header_key, header_value);
    return false;
  }
  DEBUG_MSG("Status", "Method allowed in route");
  return true;
}

void ResponseHandler::setFullPath(HttpRequest &request)
{
  // Ensure the extracted file path starts with a '/' to avoid path issues
  if (!request.file_name.empty() && request.file_name[0] != '/')
  { // this shouldn't be valid?!
    request.file_name = "/" + request.file_name;
  }

  // Set the full path to the content requested (file name might contain also a directory in it)
  request.path = request.route->path + request.file_name;
}

std::string ResponseHandler::createAllowedMethodsStr(const std::set<std::string> &methods)
{
  std::string allowed;
  for (std::set<std::string>::iterator it = methods.begin(); it != methods.end(); ++it)
  {
    allowed += *it;
    allowed += ", ";
  }
  DEBUG_MSG("Status", "crashing here?");
  if (!allowed.empty())
  {
    allowed.erase(allowed.size() - 2, 2); // Remove trailing comma and space
  }
  return allowed;
}
//--------------------------------------------------------------------------

// Populates the response object. The formatted response function is in the response class
void ResponseHandler::responseBuilder(HttpResponse &response)
{
  DEBUG_MSG_2("Status", "Building response");
  std::ostringstream oss;
  oss << response.status_code;
  DEBUG_MSG_2("Status", "Status code: " + oss.str());

  response.version = "HTTP/1.1";
  // 4xx or 5xx -> has a body with error message
  DEBUG_MSG_2("ResponseHandler::responseBuilder", "response.status_code");
  //! ANDY commented this out since I don't want to serve error pages for cgi requests, not sure if needed for non cgi requests but technically this is the correct way
  if (response.status_code >= 400)
    serveErrorPage(response); //! Todo: adjust this in a way that server error page is not created for cgi requests  
  // 200/201 -> has a body with content + content type header already filled in readFile
  //  else       -> has no body or optional (POST, DELETE)???
  DEBUG_MSG_2("ResponseHandler::responseBuilder", "serveErrorPage(response);");

  response.reason_phrase = getStatusMessage(response.status_code);

  DEBUG_MSG_2("ResponseHandler::responseBuilder", "getStatusMessage(response.status_code);");

  if (!response.body.empty())
  {
    DEBUG_MSG_2("ResponseHandler::responseBuilder", "response.body.empty() not an issue");
    if (response.headers["Content-Type"].empty())     // mandatory if body present (e.g. errors)
      response.headers["Content-Type"] = "text/html"; // use as default
    std::ostringstream oss;
    oss << response.body.length();
    response.headers["Content-Length"] = oss.str();
  }
  DEBUG_MSG_2("ResponseHandler::responseBuilder", "generateDateHeader() is an issue");

  response.headers["Date"] = generateDateHeader();
  response.headers["Server"] = "MAC_Server/1.0";
  if (response.close_connection == true && response.headers.find("Connection") == response.headers.end())
    response.headers["Connection"] = "close";
  DEBUG_MSG_2("\n..............Response complete..............\n", "");
}

std::string ResponseHandler::generateDateHeader()
{
  std::time_t now = std::time(0);
  std::tm *gmtm = std::gmtime(&now);

  char dateStr[30];
  std::strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT", gmtm);

  return std::string(dateStr);
}

void ResponseHandler::serveErrorPage(HttpResponse &response)
{
  std::string file_path = buildFullPath(response.status_code);

  if (response.is_cgi_response == false)
  {
    response.body = read_error_file(file_path);
    if (response.body.empty())
    {
      response.status_code = 500;
      return;
    }
    // alternatively we could just create the html using a template + status code & msg
    // ResponseHandler::createHtmlBody(response);
    response.close_connection = true;
    response.headers["Connection"] = "close";
  }
  return;
}

std::string ResponseHandler::buildFullPath(int status_code)
{
  std::ostringstream oss;
  oss << status_code;

  std::string full_path = ROOT_DIR;
  full_path += ERROR_PATH;
  full_path += oss.str();
  full_path += ".html";

  return full_path;
}

std::string ResponseHandler::read_error_file(std::string &file_path)
{
  DEBUG_MSG("Trying to read error file path: ", file_path);
  std::ifstream file;

  file.open(file_path.c_str(), std::ios::in | std::ios::binary);
  DEBUG_MSG_3("READ ERROR File Trying to read error file path: ", file_path);

  if (!file.is_open())
  {
    DEBUG_MSG_1("Error reading ERROR file", "");
    return "";
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();
  DEBUG_MSG_3("READ ERROR File done Trying to read error file path: ", file_path);

  file.close();

  return buffer.str();
}

//-----------------

// - 1xx: Informational - Request received, continuing process
// - 2xx: Success - The action was successfully received,
//   understood, and accepted
// - 3xx: Redirection - Further action must be taken in order to
//   complete the request
// - 4xx: Client Error - The request contains bad syntax or cannot
//   be fulfilled
// - 5xx: Server Error - The server failed to fulfill an apparently
//   valid request

// Convert status code to status message
std::string ResponseHandler::getStatusMessage(int code)
{
  switch (code)
  {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 202:
    return "Accepted";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 408:
    return "Request Time-out";
  case 415:
    return "Unsupported Media Type";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  case 504:
    return "Gateway Time-out";
  case 505:
    return "HTTP Version not supported";
  default:
    return "Unknown";
  }
}

// not needed bc we will have the custom html error pages ready to serve
//  void ResponseHandler::createHtmlBody(HttpResponse &response) {
//      //request correct and content to return
//      response.body = "<html><body><h1>";
//      response.body += "Error ";
//      std::ostringstream oss;
//      oss << response.status_code;
//      response.body += oss.str();
//      response.body += " ";
//      response.body += response.reason_phrase;
//      response.body += "</h1></body></html>";
//  }

// ----------------

// add to parser!!!

// URI Path Validation: If you haven't already, ensure that the URI path is validated early on (for malicious paths like ../ or symbols that could attempt directory traversal)
//  bool RequestParser::validatePath(HttpRequest &request) {
//      // Check for empty URI or starting character
//      if (request.uri.empty() || request.uri[0] != '/') {
//          request.error_code = 400; // 400 BAD_REQUEST
//          return false; // Invalid format
//      }
//      if (request.uri.find("..") != std::string::npos || // Directory traversal
//          request.uri.find("//") != std::string::npos)  // Double slashes
//      {
//          request.error_code = 400; // 400 BAD_REQUEST
//          return false; // Invalid path
//      }
//      // Check for invalid characters and patterns
//      const std::string forbidden_chars = "~$:*?#[{]}>|;`'\"\\ "; // "*?|<>:\"\\"
//      if (path.find_first_of(forbidden_chars) != std::string::npos) {
//        request.error_code = 400; // 400 BAD_REQUEST
//        return false; // Invalid character found}
//      }
//      if (path.length() > MAX_PATH_LENGTH)) {
//        request.error_code = 414; // 414 URI_TOO_LONG}
//        return false; // Path too long
//      }
//      return true; // URI is valid
//  }