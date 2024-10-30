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

  //needs to be adjusted since the URI might contain file name+extension etc
  if (config.routes.find(request.uri) != config.routes.end()) {
    Route route = config.routes.at(request.uri);

    request.path = route.path; // + file_name // sets the full path to the content requested
    // Verify the requested method is allowed searching in the set
    if (route.methods.find(request.method) == route.methods.end()) {
        response.status_code = 405; // Method Not Allowed
        std::string header_key = "Allow";
        std::string header_value = ResponseHandler::createAllowedMethodsStr(route.methods);
        response.setHeader(header_key, header_value);
        return;
    }

}

//--------------------------------------------------------------------------

// HELPER FUNCTIONS

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

// to fill the content type header of the response
// std::string determineContentType(const std::string& path) {
//     if (path.ends_with(".html")) return "text/html";
//     else if (path.ends_with(".ico")) return "image/x-icon";
//     else if (path.ends_with(".json")) return "application/json";
//     // Add more types as needed
//     return "text/plain";
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

//     // Check for invalid characters and patterns
//     const std::string invalidChars = "~$:*?#[{]}>|;`'\"\\ ";

//     if (request.uri.find("..") != std::string::npos || // Directory traversal
//         request.uri.find("//") != std::string::npos)  // Double slashes
//     {
//         request.error_code = 400; // 400 BAD_REQUEST
//         return false; // Invalid path
//     }

//     for (char c : request.uri) {
//         if (invalidChars.find(c) != std::string::npos) {
//             request.error_code = 400; // 400 BAD_REQUEST
//             return false; // Invalid character found
//         }
//     }

//     return true; // URI is valid
// }