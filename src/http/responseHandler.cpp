#include "responseHandler.hpp"

void ResponseHandler::processRequest(ServerConfig &config, HttpRequest &request, HttpResponse &response) {

    // from here on, we will populate & use the response object status code only
    response.status_code = request.error_code; //do at the end in populateResponse or responseBuilder

    if (response.status_code == 0) {
        // serve_file, process_api_request & populate response body (content) or error code  
        ResponseHandler::routeRequest(config, request, response);
    }

    // fill the rest of the response fields to create the final response
    // the ones with error code from parser go directly here
    ResponseHandler::responseBuilder(request, response);

}

void ResponseHandler::routeRequest(ServerConfig &config, HttpRequest &request, HttpResponse &response) {

  MimeTypeMapper mapper;
  // Find matching route in server, verify the requested method is allowed in that route and if the requested type content is allowed
  if (findMatchingRoute(config, request, response) && isMethodAllowed(request, response) && mapper.isContentTypeAllowed(request, response)) {

    if (request.is_cgi) { // at this point its been routed already and checked if (CGI)extension is allowed
        std::cout << "calling CGI handler" << std::endl;
        //CGI::handleCGIRequest(request, response);
    }
    else {
      std::cout << "calling static content handler" << std::endl;
      staticContentHandler(request, response);
    }
  }

}

void ResponseHandler::staticContentHandler(HttpRequest &request, HttpResponse &response) {

  if (request.method == "GET") {
    ResponseHandler::serveStaticFile(request, response);
  }
  else if (request.method == "POST") {
    ResponseHandler::processFileUpload(request, response);
  }
  else if (request.method == "DELETE") {
    ResponseHandler::processFileDeletion(request, response);
  }
  else { // checked already in parser
    response.status_code = 405; // Method Not Allowed
  }
}

//at this point the path is set in the request!
void ResponseHandler::serveStaticFile(HttpRequest &request, HttpResponse &response) {

  //handle directory listing first, since the next block sets response error codes
  // Default Index File: If directory listing is off, the server typically looks for a default index file, like index.html or index.php, in the requested directory. If found, it serves that file. If not found, it may return a 403 Forbidden or 404 Not Found response, depending on server configuration.
  // Directory Listing Enabled: If directory listing is enabled, and no default index file exists, the server will dynamically generate a page showing the contents of the directory, so the user can browse the files.
  if (request.is_directory) {
    request.file_name = DEFAULT_FILE; //index of all server directories
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

bool ResponseHandler::readFile(HttpRequest &request, HttpResponse &response) {

  std::ifstream file;
  file.open(request.path.c_str(), std::ios::in | std::ios::binary);

  if (!file.is_open()) { // has been checked in hasReadPermission already
      response.status_code = 500; // Internal Server Error
      return false;
  }

  // Read file into response body
  std::ostringstream buffer;
  buffer << file.rdbuf(); // Load entire file content into buffer
  response.body = buffer.str();

  file.close(); // Close the file after reading
  
  // Set status code and content type
  response.status_code = 200;

  response.setHeader("Content-Type", request.content_type);

  return true;
}

void ResponseHandler::processFileUpload(HttpRequest& request, HttpResponse& response) {

  constructFullPath(request, response);

  // Check if a file with the same name already exists at the location
  if (!fileExists(request, response)) {
    writeToFile(request, response);
  }

}

void ResponseHandler::writeToFile(HttpRequest& request, HttpResponse& response) {
  // Open the file and write request body into it
  //Permission check??
  std::ofstream file(request.path.c_str(), std::ios::binary);
  if (file.is_open()) {
      file << request.body;
      file.close();
      response.status_code = 201; // Created
      response.body = "File uploaded successfully";
      response.setHeader("Content-Type", "text/plain");
  } else {
      response.status_code = 500; // Internal Server Error
      //response.body = "500 Internal Server Error";
  }
}


void ResponseHandler::processFileDeletion(HttpRequest& request, HttpResponse& response) {
  
  constructFullPath(request, response);

  // Check if the file exists at the location. Permission check??
  if (fileExists) { // file exists
    removeFile(request, response);

  }
}

void ResponseHandler::removeFile(HttpRequest& request, HttpResponse& response) {

  if (remove(request.path.c_str()) == 0) { // try to delete
    response.status_code = 200; // OK
    //response.body = "File deleted successfully";
  } else {
    response.status_code = 500; // Internal Server Error
    //response.body = "500 Internal Server Error";
  }
}


//--------------------------------------------------------------------------

// HELPER FUNCTIONS

//make sure the path is sanitized to avoid security breaches!
//stat system call return 0 if file is accessible
bool ResponseHandler::fileExists(HttpRequest &request, HttpResponse &response) {
  struct stat path_stat;

  // // Sanitize path to avoid traversal issues (implement a sanitization function as needed)
  // if (!sanitizePath(request.path)) {
  //     response.status_code = 400; // Bad Request or similar for unsafe path
  //     return false;
  // }

  if (stat(request.path.c_str(), &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
    if (request.method == "POST") {
      response.status_code = 409; // Conflict
    }
    return true;
  }
  if (request.method == "GET" || request.method == "DELETE") {
    response.status_code = 404; // Not Found
  }
  
  return false;
}

// // simple sanitization function (not exhaustive)
// bool ResponseHandler::sanitizePath(const std::string &path) {
//     return (path.find("..") == std::string::npos); // Example check to block directory traversal
// }

void ResponseHandler::constructFullPath(HttpRequest &request, HttpResponse &response) {
  
  //std::string full_path = request.route->path; // Assuming route_path is the matched route's path
  request.path = request.route->path;

  // Check if file_name contains subdirectories (e.g., "subdir/filename.txt")
  size_t last_slash_pos = request.file_name.find_last_of('/');
  if (!handleSubdirectory(request, response, last_slash_pos))
    return;

  finalizeFullPath(request, last_slash_pos);

}

void ResponseHandler::finalizeFullPath(HttpRequest &request, size_t &last_slash_pos) {
    // Append the filename (without subdirectory) to the full path
    if (last_slash_pos != std::string::npos) {
        request.file_name = request.file_name.substr(last_slash_pos + 1);
    }
    // If no filename, extract or generate filename. Only for POST
    else if (request.method == "POST") {
      extractOrGenerateFilename(request);
    }
    
    request.path += "/" + request.file_name;

    return;
}

bool ResponseHandler::handleSubdirectory(HttpRequest &request, HttpResponse &response, size_t &last_slash_pos) {
  // Check if file_name contains subdirectories (e.g., "subdir/filename.txt")
  if (last_slash_pos != std::string::npos) {
    // Extract subdirectory path (e.g., "subdir") and update full_path
    std::string sub_dir = request.file_name.substr(0, last_slash_pos);
    request.path += "/" + sub_dir;

    // Check if the directory path exists up to this point
    struct stat path_stat;
    if (stat(request.path.c_str(), &path_stat) != 0 || !S_ISDIR(path_stat.st_mode)) {
        // Directory path does not exist
        response.status_code = 404; // Not Found
        //response.body = "The specified directory does not exist.";
        return false;
    }
  }
  return true;
}

std::string ResponseHandler::sanitizeFileName(std::string &file_name) {
    std::string sanitized;
    for (size_t i = 0; i < file_name.size(); ++i) {
        char c = file_name[i];
        if (std::isalnum(c) || c == '_' || c == '-' || (c == '.' && i > 0 && i < file_name.size() - 1)) {
            sanitized += c; // Only add allowed characters
        }
    }

    // Limit length if needed, e.g., 100 characters
    if (sanitized.length() > 100) {
        sanitized = sanitized.substr(0, 100);
    }

    return sanitized;
}

// Helper function to extract the filename from headers or generate one
void ResponseHandler::extractOrGenerateFilename(HttpRequest &request) {
  std::map<std::string, std::string>::const_iterator it = request.headers.find("Content-Disposition");
  if (it != request.headers.end()) {
    // assuming format: Content-Disposition: attachment; filename="myfile.txt"
    size_t pos = it->second.find("filename=");
    if (pos != std::string::npos) {
      request.file_name = it->second.substr(pos + 9); // Skip "filename="
      std::string no_quotes;
      for (size_t i = 0; i < request.file_name.size(); ++i) {
        if (request.file_name[i] != '\"') {
            no_quotes += request.file_name[i];
        }
      }
      request.file_name = no_quotes; // Replace original with no_quotes version
      request.file_name = ResponseHandler::sanitizeFileName(request.file_name); // Sanitize filename
    }
  }
  if (request.file_name.empty()) //generate timestamp name for avoiding repetitions
    request.file_name = generateTimestampName();
}

std::string ResponseHandler::generateTimestampName() {
    std::time_t current_time = std::time(0);

    std::ostringstream oss;
    oss << current_time;

    std::string file_name = "upload_" + oss.str();

    return file_name;
}

bool ResponseHandler::hasReadPermission(const std::string &file_path, HttpResponse &response) {

  if (access(file_path.c_str(), R_OK) == 0) // R_OK checks read access
    return true;
  else {
    response.status_code = 403; // Forbidden
    return false;
  }
}

// Store the best match if there are multiple matches (longest prefix match)
bool ResponseHandler::findMatchingRoute(const ServerConfig &config, HttpRequest &request, HttpResponse &response) {
  
  const std::map<std::string, Route> &routes = config.getRoutes();
  const Route *best_match = NULL;
  size_t longest_match_length = 0;

  for (std::map<std::string, Route>::const_iterator it = routes.begin(); it != routes.end(); ++it) {
    const std::string &route_uri = it->first;
    const Route &route_object = it->second;

  //searches for the first occurrence of the substring saved in the config route_uri within the request string uri.
    if (request.uri.find(route_uri) == 0) {
      size_t match_length = route_uri.size();//some uris might have more than one directory, so the longest match is the good one
      if (match_length > longest_match_length) {//save until there is a better match in another Route object
        best_match = &route_object;
        longest_match_length = match_length;
      }
    }
  }

  if (best_match == NULL) {
    response.status_code = 404; // Not Found
    return false;
  }

  //if route indicates a redirect, pass new location in response so that client can create a new request
  if (!best_match->redirect_uri.empty()) {
    response.status_code = 301; // Moved Permanently
    // set header specifiying new location
    std::string header_key = "Location";
    std::string header_value = best_match->redirect_uri;
    response.setHeader(header_key, header_value);
    return false;
  }
  
  request.route = best_match;
  return true;
}

bool ResponseHandler::isMethodAllowed(const HttpRequest &request, HttpResponse &response) {
  // Verify the requested method is allowed in that route searching in the set
 if (request.route->methods.find(request.method) == request.route->methods.end()) { // not found
      response.status_code = 405; // Method Not Allowed
      // set header specifiying the allowed methods
      std::string header_key = "Allow";
      std::string header_value = ResponseHandler::createAllowedMethodsStr(request.route->methods);
      response.setHeader(header_key, header_value);
      return false;
  }
  return true;
}

void ResponseHandler::setFullPath(HttpRequest &request) {
  // Ensure the extracted file path starts with a '/' to avoid path issues
  if (!request.file_name.empty() && request.file_name[0] != '/') { // this shouldn't be valid?!
    request.file_name = "/" + request.file_name;
  }

  // Set the full path to the content requested (file name might contain also a directory in it)
  request.path = request.route->path + request.file_name;
}

std::string ResponseHandler::createAllowedMethodsStr(const std::set<std::string> &methods) {
  
  std::string allowed;
  for (std::set<std::string>::iterator it = methods.begin(); it != methods.end(); ++it) {
      allowed += *it;
      allowed += ", ";
  }
  allowed.pop_back(); // Remove trailing comma
  allowed.pop_back(); // Remove trailing space
  return allowed;
}
//--------------------------------------------------------------------------

// Populates the response object. The formatted response function is in the response class
void ResponseHandler::responseBuilder(HttpRequest &request, HttpResponse &response) {
    
  response.version = "HTTP/1.1";
  //4xx or 5xx -> has a body with error message
  if (response.status_code >= 400)
    serveErrorPage(response);
  //200/201 -> has a body with content + content type header already filled in readFile
  // else       -> has no body or optional (POST, DELETE)???
  response.reason_phrase = getStatusMessage(response.status_code);

  if (!response.body.empty()) {
      if (response.headers["Content-Type"].empty()) //mandatory if body present (e.g. errors)
        response.headers["Content-Type"] = "text/html";
      std::ostringstream oss;
      oss << response.body.length();
      response.headers["Content-Length"] = oss.str(); //optional but mandatory for errors
  }
  response.headers["Date"] = generateDateHeader(); // optional
  response.headers["Server"] = "MAC_Server/1.0"; //optional
 

}

std::string ResponseHandler::generateDateHeader() {
    // Get current time
    std::time_t now = std::time(0);
    std::tm *gmtm = std::gmtime(&now); // Convert to UTC
    
    // Prepare a string to hold the formatted date
    char dateStr[30]; // Enough space for the formatted string
    
    // Format the date according to RFC 1123
    std::strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT", gmtm);
    
    return std::string(dateStr);
}

void ResponseHandler::serveErrorPage(HttpResponse &response) {

  std::string file_path = buildFullPath(response.status_code);

  response.body = read_error_file(file_path);
  if (response.body.empty()) {
      response.status_code = 500; // Internal Server Error
      return;
  }
  //alternatively we could just create the html using a template + status code & msg
  //ResponseHandler::createHtmlBody(response);
  
  response.headers["Connection"] = "close";
  
  return;
}

std::string ResponseHandler::buildFullPath(int status_code) {
    std::ostringstream oss;
    oss << status_code;  // Convert int to string

    std::string full_path = ROOT_DIR;           // Start with root directory
    full_path += ERROR_PATH;                    // Add error path
    full_path += oss.str();                     // Append the status code as string
    full_path += ".html";                       // Add file extension or suffix as needed

    return full_path;
}

std::string ResponseHandler::read_error_file(std::string &file_path) {
  std::ifstream file;
  file.open(file_path.c_str(), std::ios::in | std::ios::binary);

  if (!file.is_open()) { // has been checked in hasReadPermission already
      return ; // Internal Server Error
  }

  // Read file into response body
  std::ostringstream buffer;
  buffer << file.rdbuf(); // Load entire file content into buffer

  file.close(); // Close the file after reading
  
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
std::string ResponseHandler::getStatusMessage(int code) {
  switch (code) {
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 408: return "Request Time-out";
    case 415: return " Unsupported Media Type";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Time-out";
    case 505: return "HTTP Version not supported";
    default: return "Unknown";
  }
}

//not needed bc we will have the custom html error pages ready to serve
// void ResponseHandler::createHtmlBody(HttpResponse &response) {
//     //request correct and content to return
//     response.body = "<html><body><h1>";
//     response.body += "Error ";
//     std::ostringstream oss;
//     oss << response.status_code;
//     response.body += oss.str();
//     response.body += " ";
//     response.body += response.reason_phrase;
//     response.body += "</h1></body></html>";
// }

// ----------------



//add to parser!!!

//URI Path Validation: If you haven’t already, ensure that the URI path is validated early on (for malicious paths like ../ or symbols that could attempt directory traversal)
// bool RequestParser::validatePath(HttpRequest &request) {
//     // Check for empty URI or starting character
//     if (request.uri.empty() || request.uri[0] != '/') {
//         request.error_code = 400; // 400 BAD_REQUEST
//         return false; // Invalid format
//     }
//     if (request.uri.find("..") != std::string::npos || // Directory traversal
//         request.uri.find("//") != std::string::npos)  // Double slashes
//     {
//         request.error_code = 400; // 400 BAD_REQUEST
//         return false; // Invalid path
//     }
//     // Check for invalid characters and patterns
//     const std::string forbidden_chars = "~$:*?#[{]}>|;`'\"\\ "; // "*?|<>:\"\\"
//     if (path.find_first_of(forbidden_chars) != std::string::npos) {
//       request.error_code = 400; // 400 BAD_REQUEST
//       return false; // Invalid character found}
//     }
//     if (path.length() > MAX_PATH_LENGTH)) {
//       request.error_code = 414; // 414 URI_TOO_LONG}
//       return false; // Path too long
//     } 
//     return true; // URI is valid
// }