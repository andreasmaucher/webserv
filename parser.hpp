#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class parser;

class httpRequest {
public:
  std::string method;                         // e.g., GET, POST
  std::string uri;                            // e.g., /index.html
  std::string version;                        // e.g., HTTP/1.1
  std::map<std::string, std::string> headers; // e.g., Host, User-Agent
  std::string body; // The body of the request (optional, for POST/PUT)
};

class parser {
public:
  static std::vector<std::string>
  splitIntoLines(const std::string &raw_request);
  static void tokenizeRequestLine(const std::string &requestLine, httpRequest &request);
  static void tokenizeHeaders(std::vector<std::string>::iterator &it, const std::vector<std::string> &requestLines,  httpRequest &request);
  static void saveBody(std::vector<std::string>::iterator &it, const std::vector<std::string> &requestLines,  httpRequest &request);
};