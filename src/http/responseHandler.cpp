#include "../../include/responseHandler.hpp"
#include "../../include/cgi.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/httpResponse.hpp"

void ResponseHandler::processRequest(int &fd, Server &config, HttpRequest &request, HttpResponse &response)
{
  //! ANDY
  // Find matching route
  if (!findMatchingRoute(config, request, response))
  {
    std::cout << "No matching route found" << std::endl;
    response.status_code = 404;
    return;
  }
  const Route *route = request.route; // Route is already stored in request
  std::cout << "Route found: " << route->uri << std::endl;
  std::cout << "Is CGI route? " << (route->is_cgi ? "yes" : "no") << std::endl;
  // If it's a CGI request, handle it
  //? CGI RESPONSE request process starts here
  if (route->is_cgi)
  {
    std::cout << "Handling CGI request..." << std::endl;
    try
    {
      CGI cgi;
      cgi.handleCGIRequest(fd, request, response);

      // Always start with HTTP status line for valid HTTP/1.1 response
      response.status_code = 200;
      response.version = "HTTP/1.1";
      response.reason_phrase = "OK";

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
      else
      {
        // No headers found in CGI output
        //! MICHAEL: this seems strange - need to check
        response.body = request.body;
        response.setHeader("Content-Type", "text/plain");
      }

      response.close_connection = true;
      request.complete = true;
      return;
    }
    catch (const std::exception &e)
    {
      std::cerr << "CGI execution failed: " << e.what() << std::endl;
      response.status_code = 500;
      response.body = "CGI execution failed";
      response.close_connection = true;
      request.complete = true; // Make sure request is marked complete
      return;
    }
  }

  /* -- Carinas part starts here -- */
  std::cout << "Processing request" << std::endl;
  // from here on, we will populate & use the response object status code only
  // response.status_code = request.error_code; //do at the end in populateResponse or responseBuilder
  // find connection header and set close_connection in response object
  if (request.error_code == 0)
  { // Check error_code but don't set response status
    ResponseHandler::routeRequest(fd, config, request, response);
  }
  if (response.status_code == 0)
  {
    // serve_file, process_api_request & populate response body (content) or error code
    // ResponseHandler::routeRequest(config, request, response);
    response.status_code = 200;
  }

  if (request.headers.find("Connection") != request.headers.end() && request.headers["Connection"] == "close")
    response.close_connection = true;
  // fill the rest of the response fields to create the final response
  // the ones with error code from parser go directly here
  // to do: (check if in the parser I set some other value like headers etc since here I'm not passing the request object)
  ResponseHandler::responseBuilder(response);
}

void ResponseHandler::routeRequest(int &fd, Server &config, HttpRequest &request, HttpResponse &response)
{
  std::cout << "Routing request" << std::endl;
  MimeTypeMapper mapper;
  // Find matching route in server, verify the requested method is allowed in that route and if the requested type content is allowed
  if (findMatchingRoute(config, request, response) && isMethodAllowed(request, response) && mapper.isContentTypeAllowed(request, response))
  {
    if (request.is_cgi)
    { // at this point its been routed already and checked if (CGI)extension is allowed
      std::cout << "calling CGI handler" << std::endl;
      //! CGI PROCESS STARTS
      CGI cgiHandler;
      cgiHandler.handleCGIRequest(fd, request, response);
    }
    else
    {
      std::cout << "calling static content handler" << std::endl;
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

// at this point the path is set in the request!
void ResponseHandler::serveStaticFile(HttpRequest &request, HttpResponse &response)
{
  std::cout << "Serve static file" << std::endl;
  // handle directory listing first, since the next block sets response error codes
  //  Default Index File: If directory listing is off, the server typically looks for a default index file, like index.html or index.php, in the requested directory. If found, it serves that file. If not found, it may return a 403 Forbidden or 404 Not Found response, depending on server configuration.
  //  Directory Listing Enabled: If directory listing is enabled, and no default index file exists, the server will dynamically generate a page showing the contents of the directory, so the user can browse the files.
  if (request.is_directory)
  {
    request.file_name = DEFAULT_FILE; // index of all server directories
    request.is_directory = false;     // to avoid the fileExists directory check
    // if (config.routes[request.uri].directory_listing) {
    //   std::cout << "Implement directory listing" << std::endl;
    //   //search for & serve default index file or generate dynamically
    //   // Generate directory listing
    // }
    // else {
    //   //search for & serve default index file or return error
    //   response.status_code = 403;  // Forbidden if no file is found and listing is off or Not Found
    // }
  }
  ResponseHandler::setFullPath(request);

  if (ResponseHandler::fileExists(request, response) && ResponseHandler::hasReadPermission(request.path, response))
    readFile(request, response);
}

bool ResponseHandler::readFile(HttpRequest &request, HttpResponse &response)
{
  std::ifstream file;
  file.open(request.path.c_str(), std::ios::in | std::ios::binary);

  if (!file.is_open())
  {
    response.status_code = 500;
    return false;
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();
  response.body = buffer.str();
  file.close();
  response.status_code = 200;
  response.setHeader("Content-Type", request.content_type);

  return true;
}

void ResponseHandler::processFileUpload(HttpRequest &request, HttpResponse &response)
{
  std::cout << "Processing file upload" << std::endl;

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
    std::cout << "File uploaded successfully" << std::endl;
  }
  else
  {
    std::cout << "Error writing to file" << std::endl;
    response.status_code = 500;
  }
}

void ResponseHandler::processFileDeletion(HttpRequest &request, HttpResponse &response)
{
  std::cout << "Processing file deletion" << std::endl;
  constructFullPath(request, response);

  // Check if the file exists at the location. Permission check??
  if (fileExists(request, response))
  {
    std::cout << "Deleting file" << std::endl;
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
  if (stat(request.path.c_str(), &path_stat) == 0 && (!request.is_directory && S_ISREG(path_stat.st_mode)))
  {
    std::cout << "File exists and accessible" << std::endl;
    if (request.method == "POST")
    {
      std::cout << "File already exists" << std::endl;
      response.status_code = 409;
    }
    return true;
  }
  if (request.method == "GET" || (request.method == "DELETE" && !request.is_directory))
  {
    std::cout << "File does not exist" << std::endl;
    response.status_code = 404;
  }
  else if (request.method == "DELETE" && request.is_directory)
  {
    std::cout << "Directory deletion not implemented" << std::endl;
    response.status_code = 501;
  }
  std::cout << "File does not exist or not accessible" << std::endl;
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
    std::cout << "File name: " << request.file_name << std::endl;
  }
  // If no filename, extract or generate filename. Only for POST
  else if (request.method == "POST")
  {
    extractOrGenerateFilename(request);
  }

  request.path += "/" + request.file_name;

  std::cout << "Full path to the conent: " << request.path << std::endl;

  return;
}

bool ResponseHandler::handleSubdirectory(HttpRequest &request, HttpResponse &response, size_t &last_slash_pos)
{
  // Check if file_name contains subdirectories (e.g., "subdir/filename.txt")
  if (last_slash_pos != std::string::npos)
  {
    // Extract subdirectory path (e.g., "subdir") and update full_path
    std::string sub_dir = request.file_name.substr(0, last_slash_pos);
    std::cout << "Subdirectory found: " << sub_dir << std::endl;
    request.path += "/" + sub_dir;

    // Check if the directory path exists up to this point
    struct stat path_stat;
    if (stat(request.path.c_str(), &path_stat) != 0 || !S_ISDIR(path_stat.st_mode))
    {
      std::cout << "Subdirectory/path does not exist" << std::endl;
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
      std::cout << "Extracted file name: " << request.file_name << std::endl;
    }
  }
  if (request.file_name.empty())
  {
    request.file_name = generateTimestampName();
    std::cout << "Generated file name with timestamp: " << request.file_name << std::endl;
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
  std::cout << "Finding matching route for [" << request.uri << "]" << std::endl;
  const std::map<std::string, Route> &routes = server.getRoutes();
  const Route *best_match = NULL;
  size_t longest_match_length = 0;

  // Debug output for all routes
  std::cout << "Available routes:" << std::endl;
  for (std::map<std::string, Route>::const_iterator it = routes.begin(); it != routes.end(); ++it)
  {
    std::cout << "Route: [" << it->first << "] CGI: " << (it->second.is_cgi ? "Yes" : "No") << std::endl;
  }

  for (std::map<std::string, Route>::const_iterator it = routes.begin(); it != routes.end(); ++it)
  {
    const std::string &route_uri = it->first;
    std::cout << "Comparing request [" << request.uri << "] to route [" << route_uri << "]" << std::endl;
    const Route &route_object = it->second;

    // Modified matching logic to handle CGI paths better
    bool is_match = false;
    if (route_object.is_cgi)
    {
      // For CGI routes, check if the URI starts with the route path
      is_match = (request.uri.compare(0, route_uri.size(), route_uri) == 0);
    }
    else
    {
      // For regular routes, use the existing logic
      is_match = (request.uri.compare(0, route_uri.size(), route_uri) == 0 &&
                  (request.uri.size() == route_uri.size() || request.uri[route_uri.size()] == '/'));
    }

    if (is_match)
    {
      size_t match_length = route_uri.size();
      if (match_length > longest_match_length)
      {
        best_match = &route_object;
        longest_match_length = match_length;
        std::cout << "Found better match: [" << route_uri << "]" << std::endl;
      }
    }
  }

  if (best_match == NULL)
  {
    std::cout << "No matching route found" << std::endl;
    response.status_code = 404;
    return false;
  }

  std::cout << "Best matching route: [" << best_match->uri << "] CGI: " << (best_match->is_cgi ? "Yes" : "No") << std::endl;
  request.route = best_match;
  request.is_cgi = best_match->is_cgi;
  return true;
}

bool ResponseHandler::isMethodAllowed(const HttpRequest &request, HttpResponse &response)
{
  std::cout << "Checking if method " << request.method << " is allowed" << std::endl;
  // Verify the requested method is allowed in that route searching in the set
  if (request.route->methods.find(request.method) == request.route->methods.end())
  {
    std::cout << "Method not allowed in route" << std::endl;
    response.status_code = 405;
    std::string header_key = "Allow";
    std::string header_value = ResponseHandler::createAllowedMethodsStr(request.route->methods);
    response.setHeader(header_key, header_value);
    return false;
  }
  std::cout << "Method allowed in route" << std::endl;
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
  std::cout << "crashing here?" << std::endl;
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
  std::cout << "Building response" << std::endl;

  std::cout << "Status code: " << response.status_code << std::endl;

  response.version = "HTTP/1.1";
  // 4xx or 5xx -> has a body with error message
  if (response.status_code >= 400)
    serveErrorPage(response);
  // 200/201 -> has a body with content + content type header already filled in readFile
  //  else       -> has no body or optional (POST, DELETE)???
  response.reason_phrase = getStatusMessage(response.status_code);

  if (!response.body.empty())
  {
    if (response.headers["Content-Type"].empty())     // mandatory if body present (e.g. errors)
      response.headers["Content-Type"] = "text/html"; // use as default
    std::ostringstream oss;
    oss << response.body.length();
    response.headers["Content-Length"] = oss.str();
  }
  response.headers["Date"] = generateDateHeader();
  response.headers["Server"] = "MAC_Server/1.0";
  if (response.close_connection == true && response.headers.find("Connection") == response.headers.end())
    response.headers["Connection"] = "close";

  std::cout << "\n..............Response complete..............\n"
            << std::endl;
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
  std::cout << "Trying to read error file path: " << file_path << std::endl;
  std::ifstream file;

  file.open(file_path.c_str(), std::ios::in | std::ios::binary);

  if (!file.is_open())
  {
    std::cout << "Error reading ERROR file" << std::endl;
    return "";
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();
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