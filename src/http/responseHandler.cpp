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
    ResponseHandler::populateResponse(request, response);

}

void ResponseHandler::routeRequest(ServerConfig &config, HttpRequest &request, HttpResponse &response) {

  MimeTypeMapper mapper;
  // Find matching route in server, verify the requested method is allowed in that route and if the requested type content is allowed
  if (findMatchingRoute(config, request, response) && isMethodAllowed(request, response) && mapper.isContentTypeAllowed(request, response)) {

    std::string file_name = mapper.getFileName(request);
    ResponseHandler::setPathToContent(request, file_name);

    if (mapper.isCGIRequest(request)) { // at this point its been routed already and checked if (CGI)extension is allowed
      //if (request.route->is_cgi) { // not needed, has been checked in the mapper
        std::cout << "calling CGI handler" << std::endl;
        //CGI::handleCGIRequest(request, response);
    }
    else {
      std::cout << "calling static content handler" << std::endl;
      //staticContentHandler(request, response);
    }
  }

}

void staticContentHandler(HttpRequest &request, HttpResponse &response) {

  if (request.method == "GET") {
    serveStaticFile(request, response);
  }
  // else if (request.method == "POST") {
  //   handleFileUpload(request, response);
  // }
  // else if (request.method == "DELETE") {
  //   handleDeleteFile(request, response);
  // }
  else { // checked already in parser
    response.status_code = 405; // Method Not Allowed
  }
}

//at this point the path is set in the request!
void serveStaticFile(HttpRequest &request, HttpResponse &response) {

  MimeTypeMapper mapper;


  if (!fileExists(file_path)) {
      response.setStatusCode(404); // Not Found
      return false;
  }


  if (file_exists(file_path, response) && has_read_permissions(file_path, response)) {
    response.file_content = read_file(file_path);
    response.status_code = 200;
   
    std::string content_type = mapper.getContentType(mapper.getFileExtension(request));
    response.setHeader("Content-Type", content_type);
  }
  else {
    response.status_code = 404; // Not Found
  }
}

// bool serveStaticFile(const std::string &file_path, HttpResponse &response) {
//     // Check if the file exists and can be opened
//     if (!fileExists(file_path)) {
//         response.setStatusCode(404); // Not Found
//         return false;
//     }

//     std::ifstream file(file_path.c_str(), std::ios::in | std::ios::binary);
//     if (!file.is_open()) {
//         response.setStatusCode(403); // Forbidden
//         return false;
//     }

//     // Read file into response body
//     std::ostringstream buffer;
//     buffer << file.rdbuf(); // Load entire file content into buffer
//     response.body = buffer.str();

//     // Set status code and content type
//     response.setStatusCode(200);
//     std::string extension = getFileExtension(file_path);
//     response.setHeader("Content-Type", getContentType(extension));

//     file.close(); // Close the file after reading
//     return true;
// }



//--------------------------------------------------------------------------

// HELPER FUNCTIONS

//make sure the path is sanitized to avoid security breaches!
//stat system call return 0 if file is accessible
bool fileExists(const std::string &path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Store the best match if there are multiple matches (longest prefix match)
bool ResponseHandler::findMatchingRoute(const ServerConfig &config, HttpRequest &request, HttpResponse &response) {
  
  const Route *best_match = NULL;
  size_t longest_match_length = 0;

  for (std::map<std::string, Route>::const_iterator it = config.routes.begin(); it != config.routes.end(); ++it) {
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

void ResponseHandler::setPathToContent(HttpRequest &request, std::string &file_name) {
  // Ensure the extracted file path starts with a '/' to avoid path issues
  if (!file_name.empty() && file_name[0] != '/') {
    file_name = "/" + file_name;
  }

  // Set the full path to the content requested
  request.path = request.route->path + file_name;
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


// void ResponseHandler::handleGet(HttpRequest &request, HttpResponse &response) {

//   if (request.uri == "/" || request.uri == "/index.html") {
//     response.content = serveStaticFile(DEFAULT_FILE);
//     if (response.content.empty()) {
//         response.status_code = 404;
//         return;
//     }
//     response.status_code = 200;
//     return;
//   }
//   else if request_path == "/favicon.ico"
//       serve_file(favicon.ico)
//       send_response(200, "OK", "image/x-icon", favicon_ico)
//   else
//       if path does not exist
//           send_error_response(404)
//       if method no permissions
//           send_error_response(403)    
// }

// // include Content-Type Validation
// void ResponseHandler::serveStaticFile(std::string &uri, HttpResponse &response) {
  
//   std::string path = ROOT_DIR + '/' + uri;

//   //set status code in validation functions to indicate error
//   if (is_file(path, response) && file_exists(path, response) && has_read_permissions(path, response)) {
//     content = read_file(path)
//   }
  
//   return;

// }


// void ResponseHandler::serveErrorPage(HttpResponse &response){
//
//   std::string file_path = ROOT_DIR + "/html/errors/" + response.status_code + ".html";
//
//   response.content = read_file(file_path);
//   return;
// }

// void handleFileUpload(const HttpRequest& request, HttpResponse& response) {
//     // was checked in the parser alredy
//     // // Ensure request has body data
//     // if (request.body.empty()) {
//     //     response.status_code = 400; // Bad Request
//     //     response.body = "400 Bad Request: No data in request body";
//     //     return;
//     // }

//     // Process upload (e.g., save to server)
//     std::ofstream file("/var/uploads/uploaded_file.txt");
//     if (file.is_open()) {
//         file << request.body;
//         file.close();
//         response.status_code = 201; // Created
//         response.body = "File uploaded successfully";
//     } else {
//         response.status_code = 500; // Internal Server Error
//         response.body = "500 Internal Server Error";
//     }
// }

// void handleDeleteFile(const HttpRequest& request, HttpResponse& response) {
//     std::string path = "/var/uploads" + request.uri; // example path resolution

//     if (remove(path.c_str()) == 0) {
//         response.status_code = 200;
//         response.body = "File deleted successfully";
//     } else {
//         response.status_code = 404; // Not Found
//         response.body = "404 Not Found";
//     }
// }

// if error -> has a body with error message
// if 200/201 -> has a body with content
//             -> has no body (POST, DELETE)???


// // Populates the response object. The formatted response function is in the response class
// void ResponseHandler::populateResponse(HttpRequest &request, HttpResponse &response) {
    
//     response.version = "HTTP/1.1";

//     response.reason_phrase = getStatusMessage(response.status_code);

//     ResponseHandler::createHtmlBody(response);

//     if (response.file_content.empty()) {
//         response.headers["Content-Type"] = "text/html";
//         response.headers["Content-Length"] = std::to_string(response.body.length());
//     }
//     else {
//       response.headers["Content-Type"] = response.file_content_type;
//       response.headers["Content-Length"] = response.body.length();
//     } 

// }

//not needed bc we will have the custom html error pages ready to serve
// void ResponseHandler::createHtmlBody(HttpResponse &response) {

//     // request correct but no content to return (e.g., DELETE or POST)

//     //request correct and content to return

//     response.body = "<html><body><h1>";

//     if (response.status_code != 200) {
//         response.body += "Error ";
//         std::ostringstream oss;
//         oss << response.status_code;

//         response.body += oss.str();
//         response.body += " ";
//         response.body += response.reason_phrase;
//     }

//     else {
//       //response.body += response.content;
//       response.body += "Hello, World!";
//     }

//     response.body += "</h1></body></html>";
// }


// Convert status code to status message
// std::string ResponseHandler::getStatusMessage(int code) {
//   switch (code) {
//     case 200: return "OK";
//     case 201: return "Created";
//     case 202: return "Accepted";
//     case 204: return "No Content";
//     case 301: return "Moved Permanently";
//     case 400: return "Bad Request";
//     case 401: return "Unauthorized";
//     case 403: return "Forbidden";
//     case 404: return "Not Found";
//     case 405: return "Method Not Allowed";
//     case 408: return "Request Time-out";
//     case 415: return " Unsupported Media Type";
//     case 500: return "Internal Server Error";
//     case 501: return "Not Implemented";
//     case 502: return "Bad Gateway";
//     case 503: return "Service Unavailable";
//     case 504: return "Gateway Time-out";
//     case 505: return "HTTP Version not supported";
//     default: return "Unknown";
//   }
// }

// ----------------

/*    - 1xx: Informational - Request received, continuing process
      - 2xx: Success - The action was successfully received,
        understood, and accepted
      - 3xx: Redirection - Further action must be taken in order to
        complete the request
      - 4xx: Client Error - The request contains bad syntax or cannot
        be fulfilled
      - 5xx: Server Error - The server failed to fulfill an apparently
        valid request
*/
//-----------------


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