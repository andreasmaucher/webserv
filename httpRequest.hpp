#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class HttpRequest {
public:
  std::string method;                         // e.g., GET, POST
  std::string uri;                            // e.g., /index.html
  std::string version;                        // e.g., HTTP/1.1
  std::map<std::string, std::string> headers; // e.g., Host, User-Agent
  std::string body; // The body of the request (optional, for POST/PUT)
  int error_code; 
};

#endif