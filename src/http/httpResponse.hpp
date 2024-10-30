#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Core data structure for outgoing responses
class HttpResponse {
public:
    HttpResponse();
    
    std::string version;                        // e.g., HTTP/1.1
    int status_code;                            // e.g., 200, 404
    std::string reason_phrase;                  // e.g., OK, Not Found
    std::map<std::string, std::string> headers; // e.g., Content-Type: text/html
    std::string body;                           // The body of the response

    std::string file_content;                   // e.g., <html><body><h1>Hello, World!</h1></body></html>   
    std::string file_content_type;              // e.g., text/html
    std::string generateResponseStr();

    void setHeader(std::string &header_name, std::string &header_value);
};

#endif

//HTTP RESPONSE FORMAT:
// HTTP-Version Status-Code Status-Message
// Header-Name: Header-Value
// blank line
// Body (optional)

//example:
// HTTP/1.1 200 OK
// Date: Mon, 21 Oct 2024 10:00:00 GMT
// Content-Type: text/html
// Content-Length: 88
// Connection: keep-alive

// <html>
//   <body>
//     <h1>Hello, World!</h1>
//   </body>
// </html>


//Always mandatory:
//Status line: HTTP-Version Status-Code Status-Message

//Common headers:
// Content-Type: Specifies the media type of the body (e.g., Content-Type: text/html).Mandatory for msgs with body.
// Content-Length: Indicates the length of the response body in bytes. (or Transfer-Encoding: chunked)
// Connection: Whether the server will close the connection or keep it alive (Connection: close or Connection: keep-alive).
//Other recommended ones:
// Date: The date and time when the response was generated.
// Server: The name and version of the server software (e.g., Server: Apache/2.4.39 (Unix)).

//Requiered for error responses:
//Content-Type
//Content-Length
//Connection: close
//Body: HTML error page