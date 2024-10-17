#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "httpRequest.hpp"

class RequestParser {
public:
  //static std::vector<std::string> splitIntoLines(const std::string &raw_request);

  static bool parseRawRequest(HttpRequest &request, const std::string &raw_request, size_t &position);
  static void tokenizeRequestLine(HttpRequest &request, const std::string &raw_request, size_t &position);
  static void tokenizeHeaders(HttpRequest &request, const std::string &raw_request, size_t &position);
  static bool saveValidBody(HttpRequest &request, const std::string &raw_request, size_t &position);

  static std::string readLine(const std::string &raw_request, size_t &position);
  static bool validRequestLine(HttpRequest &request);
  static bool validMethod(HttpRequest &request);
  static bool validPathFormat(HttpRequest &request);
  static bool validHttpVersion(HttpRequest &request);
  static bool validHeaderFormat(std::map<std::string, std::string> &headers, const std::string &current_line, size_t colon_pos);
};

#endif