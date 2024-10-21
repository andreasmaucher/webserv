#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#define MAX_BODY_SIZE 1000000

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "httpRequest.hpp"

class RequestParser {
public:

  static bool parseRawRequest(HttpRequest &request, const std::string &raw_request, size_t &position);
  static void tokenizeRequestLine(HttpRequest &request, const std::string &raw_request, size_t &position);
  static void tokenizeHeaders(HttpRequest &request, const std::string &raw_request, size_t &position);
  static bool parseBody(HttpRequest &request, const std::string &raw_request, size_t &position);
  static bool saveChunkedBody(HttpRequest &request, const std::string &raw_request, size_t &position);
  static bool saveContentLengthBody(HttpRequest &request, const std::string &raw_request, size_t &position);

  static std::string readLine(const std::string &raw_request, size_t &position);
  static bool isBodyExpected(HttpRequest &request);
  static bool validRequestLine(HttpRequest &request);
  static bool validMethod(HttpRequest &request);
  static bool validPathFormat(HttpRequest &request);
  static bool validHttpVersion(HttpRequest &request);
  static bool validHeaderFormat(std::map<std::string, std::string> &headers, const std::string &current_line, size_t colon_pos);
};

#endif