#include "responseHandler.hpp"

void ResponseHandler::processRequest(HttpRequest &request, HttpResponse &response) {

    if (request.error_code == 0) {
        // serve_file, process_api_request & populate response body (content) or error code  
        ResponseHandler::routeRequest(request, response);
    }

    // fill the rest of the response fields
    // the ones with error code from parser go directly here
    ResponseHandler::populateResponse(request, response);

}

void ResponseHandler::routeRequest() {
    if request_method == GET
        error = serve_file(request.uri, response.content)
        if (error == 0)
            send_response(200, "OK", request.headers["Content-Type"], file_content)
        else
            send_error_response(error)
    
    else if request_method == POST
        if request_path == "/api"
            api_response = process_api_request(request_body)
            send_response(200, "OK", "application/json", api_response)
        else
            send_error_response(404)
    else // if DELETE
        try to go to directory
            if permissions ok
                delete file
                send_response(200, "OK", "text/html", "File deleted")
            else
                send_error_response(403)
}

// if request_path == "/" || request_path == "/index.html"
//             response.body = read_file(index.html)
//             send_response(200, "OK", "text/html", index_html)
// else if request_path == "/favicon.ico"
//     serve_file(favicon.ico)
//     send_response(200, "OK", "image/x-icon", favicon_ico)
// else
//     if path does not exist
//         send_error_response(404)
//     if method no permissions
//         send_error_response(403)    

// // include Content-Type Validation
// void serveStaticFile(uri, content){
// if file_exists(uri)
//     content = read_file(uri)
//     return 0
// else
//     return 404
// }

// void serveErrorPage(error_code, content){
//   switch (code) {
//       case 400:
//           content = read_file("400.html")
//           return 0
//       case 404:
//           content = read_file("404.html")
//           return 0
//       default:
//           content = read_file("404.html")
//           return 0;
//   }
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

// void ResponseHandler::populateResponse(HttpRequest &request, HttpResponse &response) {
    
//     response.version = "HTTP/1.1";
//     response.status_code = request.error_code;
//     response.reason_phrase = getStatusMessage(request.error_code);
//     response.body = ResponseHandler::createHtmlBody(response);
//     response.headers["Content-Type"] = content_type;
//     response.headers["Content-Length"] = body.length();

// }

void ResponseHandler::createHtmlBody(HttpResponse &response) {

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
