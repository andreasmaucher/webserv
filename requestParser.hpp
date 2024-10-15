#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "httpRequest.hpp"

class parser {
public:
  static std::vector<std::string>
  splitIntoLines(const std::string &raw_request);
  static void tokenizeRequestLine(const std::string &requestLine, httpRequest &request);
  static void tokenizeHeaders(std::vector<std::string>::iterator &it, const std::vector<std::string> &requestLines,  httpRequest &request);
  static void saveBody(std::vector<std::string>::iterator &it, const std::vector<std::string> &requestLines,  httpRequest &request);
};

#endif