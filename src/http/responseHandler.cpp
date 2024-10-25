#include "responseHandler.hpp"

void ResponseHandler::processRequest(HttpRequest &request, HttpResponse &response) {

    response.status_code = request.error_code;

    if (response.status_code == 0) {
        // serve_file, process_api_request & populate response body (content) or error code  
        ResponseHandler::routeRequest(request, response);
    }

    // fill the rest of the response fields
    // the ones with error code from parser go directly here
    ResponseHandler::populateResponse(request, response);

}

void ResponseHandler::routeRequest(HttpRequest &request, HttpResponse &response) {
  
  if (request.method == "GET") {
    ResponseHandler::handleGet(request, response);
  }

  else if (request.method == "POST") {
    ResponseHandler::handlePost(request, response);
    // if request_path == "/api"
    //     api_response = process_api_request(request_body)
    //     send_response(200, "OK", "application/json", api_response)
    // else
    //     send_error_response(404)
  }

  else // if DELETE
    ResponseHandler::handleDelete(request, response);
    // try to go to directory
    //     if permissions ok
    //         delete file
    //         send_response(200, "OK", "text/html", "File deleted")
    //     else
    //         send_error_response(403)
}

void ResponseHandler::handleGet(HttpRequest &request, HttpResponse &response) {

  if (request.uri == "/" || request.uri == "/index.html") {
    response.content = serveStaticFile(DEFAULT_FILE);
    if (response.content.empty()) {
        response.status_code = 404;
        return;
    }
    response.status_code = 200;
    return;
  }
  else if request_path == "/favicon.ico"
      serve_file(favicon.ico)
      send_response(200, "OK", "image/x-icon", favicon_ico)
  else
      if path does not exist
          send_error_response(404)
      if method no permissions
          send_error_response(403)    
}

// include Content-Type Validation
void ResponseHandler::serveStaticFile(std::string &uri, HttpResponse &response) {
  
  std::string path = ROOT_DIR + '/' + uri;

  //set status code in validation functions to indicate error
  if (is_file(path, response) && file_exists(path, response) && has_read_permissions(path, response)) {
    content = read_file(path)
  }
  
  return;

}


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


// Populates the response object. The formatted response function is in the response class
void ResponseHandler::populateResponse(HttpRequest &request, HttpResponse &response) {
    
    response.version = "HTTP/1.1";

    response.reason_phrase = getStatusMessage(response.status_code);

    ResponseHandler::createHtmlBody(response);

    if (response.file_content.empty()) {
        response.headers["Content-Type"] = "text/html";
        response.headers["Content-Length"] = std::to_string(response.body.length());
    }
    else {
      response.headers["Content-Type"] = response.file_content_type;
      response.headers["Content-Length"] = response.body.length();
    } 

}

void ResponseHandler::createHtmlBody(HttpResponse &response) {

    // request correct but no content to return (e.g., DELETE or POST)

    //request correct and content to return

    response.body = "<html><body><h1>";

    if (response.status_code != 200) {
        response.body += "Error ";
        std::ostringstream oss;
        oss << response.status_code;

        response.body += oss.str();
        response.body += " ";
        response.body += response.reason_phrase;
    }

    else {
      //response.body += response.content;
      response.body += "Hello, World!";
    }

    response.body += "</h1></body></html>";
}

// // Convert status code to status message
// std::string ResponseHandler::getStatusMessage(int code) {
//     switch (code) {
//         case 200: return "OK";
//         case 404: return "Not Found";
//         case 400: return "Bad Request";
//         // Add other statuses as needed
//         default: return "Unknown";
//     }
// }

// ----------------

/*      - 1xx: Informational - Request received, continuing process
      - 2xx: Success - The action was successfully received,
        understood, and accepted
      - 3xx: Redirection - Further action must be taken in order to
        complete the request
      - 4xx: Client Error - The request contains bad syntax or cannot
        be fulfilled
      - 5xx: Server Error - The server failed to fulfill an apparently
        valid request

Status-Code    =
            "100"  ; Section 10.1.1: Continue
          | "101"  ; Section 10.1.2: Switching Protocols
          | "200"  ; Section 10.2.1: OK
          | "201"  ; Section 10.2.2: Created
          | "202"  ; Section 10.2.3: Accepted
          | "203"  ; Section 10.2.4: Non-Authoritative Information
          | "204"  ; Section 10.2.5: No Content
          | "205"  ; Section 10.2.6: Reset Content
          | "206"  ; Section 10.2.7: Partial Content
          | "300"  ; Section 10.3.1: Multiple Choices
          | "301"  ; Section 10.3.2: Moved Permanently
          | "302"  ; Section 10.3.3: Found
          | "303"  ; Section 10.3.4: See Other
          | "304"  ; Section 10.3.5: Not Modified
          | "305"  ; Section 10.3.6: Use Proxy
          | "307"  ; Section 10.3.8: Temporary Redirect
          | "400"  ; Section 10.4.1: Bad Request
          | "401"  ; Section 10.4.2: Unauthorized
          | "402"  ; Section 10.4.3: Payment Required
          | "403"  ; Section 10.4.4: Forbidden
          | "404"  ; Section 10.4.5: Not Found
          | "405"  ; Section 10.4.6: Method Not Allowed
          | "406"  ; Section 10.4.7: Not Acceptable
          | "407"  ; Section 10.4.8: Proxy Authentication Required
          | "408"  ; Section 10.4.9: Request Time-out
          | "409"  ; Section 10.4.10: Conflict
          | "410"  ; Section 10.4.11: Gone
          | "411"  ; Section 10.4.12: Length Required
          | "412"  ; Section 10.4.13: Precondition Failed
          | "413"  ; Section 10.4.14: Request Entity Too Large
          | "414"  ; Section 10.4.15: Request-URI Too Large
          | "415"  ; Section 10.4.16: Unsupported Media Type
          | "416"  ; Section 10.4.17: Requested range not satisfiable
          | "417"  ; Section 10.4.18: Expectation Failed
          | "500"  ; Section 10.5.1: Internal Server Error
          | "501"  ; Section 10.5.2: Not Implemented
          | "502"  ; Section 10.5.3: Bad Gateway
          | "503"  ; Section 10.5.4: Service Unavailable
          | "504"  ; Section 10.5.5: Gateway Time-out
          | "505"  ; Section 10.5.6: HTTP Version not supported
          | extension-code
*/

//-----------------

// // add to connection class (communication layer) and call from recv loop
// void sendResponse(int clientSocket, HTTPResponse &response) {
//     send(clientSocket, responseStr.c_str(), responseStr.size(), 0);
// }


//add to parser!!!
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