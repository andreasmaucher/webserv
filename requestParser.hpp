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
  static std::vector<std::string> splitIntoLines(const std::string &raw_request);
  static void tokenizeRequestLine(const std::string &request_line, HttpRequest &request);
  static void tokenizeHeaders(std::vector<std::string>::iterator &it, const std::vector<std::string> &request_lines,  HttpRequest &request);
  static void saveValidBody(std::vector<std::string>::iterator &it, const std::vector<std::string> &request_lines,  HttpRequest &request);

  static bool validRequestLine(HttpRequest &request);
  static bool validMethod(HttpRequest &request);
  static bool validPathFormat(HttpRequest &request);
  static bool validHttpVersion(HttpRequest &request);
  static bool validHeaderFormat(std::map<std::string, std::string> &headers, const std::string &current_line, size_t colon_pos);
};

#endif