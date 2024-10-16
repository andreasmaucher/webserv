#include "requestParser.hpp"

// Split the raw request into lines (raw_request = buffer passed to recv())
std::vector<std::string> RequestParser::splitIntoLines(const std::string &raw_request) {
  std::istringstream stream(raw_request);
  std::string line;
  std::vector<std::string> lines;

  while (std::getline(stream, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1); // Remove '\r'
    }
    lines.push_back(line);
  }
  
  return lines;
}

// Extract method, URI, and version from the request line
void RequestParser::tokenizeRequestLine(const std::string &request_line, HttpRequest &request) {
  if (request_line.empty()){
    request.error_code = 400; //400 BAD_REQUEST
    throw std::runtime_error("Empty request line");
    //return;
  }
  
  std::istringstream stream(request_line);
  
  stream >> request.method >> request.uri >> request.version;
  if (stream.fail() || !validRequestLine(request)) {
    return;
  }
}

// Extract headers from request until blank line (\r\n)
void RequestParser::tokenizeHeaders(std::vector<std::string>::iterator &it, const std::vector<std::string> &request_lines,  HttpRequest &request) {
  if (it->empty())
    return;

  while (!(++it)->empty() && it != request_lines.end()) {
    std::string current_line = *it;
    size_t colon_pos = current_line.find(":");
    
    if (!validHeaderFormat(request.headers, current_line, colon_pos)) {
      request.error_code = 400; //400 BAD_REQUEST
      throw std::runtime_error("Bad header format"); // bad formatted header (twice same name, no ":")
      //return;
    }
  }
}

void RequestParser::saveValidBody(std::vector<std::string>::iterator &it, const std::vector<std::string> &request_lines,  HttpRequest &request) {

  if (!(++it)->empty() && it != request_lines.end()) {
    
    std::map<std::string, std::string>::iterator iter = request.headers.find("Content-Length");
    
    if (iter != request.headers.end()) {
      
      std::istringstream iss(iter->second);
      size_t content_length;
      iss >> content_length; // Get the integer value of content length
      
      if (content_length != (*it).size()) {
        request.error_code = 400; //400 BAD_REQUEST
        throw std::runtime_error("Invalid body length");
        //return false;
      }
    }

   request.body = *it;

  }
}

bool RequestParser::validRequestLine(HttpRequest &request)
{
  return (validMethod(request) && validPathFormat(request) && validHttpVersion(request));  
}

bool RequestParser::validMethod(HttpRequest &request)
{
  if (!request.method.empty() && (request.method == "GET" || request.method == "POST" || request.method == "DELETE"))
  {
    return true;
  }
  request.error_code = 405; //405 METHOD_NOT_ALLOWED
  throw std::runtime_error("Invalid method");
  //return false;
}

bool RequestParser::validPathFormat(HttpRequest &request)
{
  if (request.uri.empty() || request.uri[0] != '/' || request.uri.find(" ") != std::string::npos)
  {
    request.error_code = 400; //400 BAD_REQUEST
    throw std::runtime_error("Invalid path format");
    //return false;
  }
  return true;
}

bool RequestParser::validHttpVersion(HttpRequest &request)
{
  if (!request.version.empty() && request.version == "HTTP/1.1")
  {
    return true;
  }
  request.error_code = 505; //505 HTTP_VERSION_NOT_SUPPORTED
  throw std::runtime_error("Invalid HTTP version");
  //return false;
}

bool RequestParser::validHeaderFormat(std::map<std::string, std::string> &headers, const std::string &current_line, size_t colon_pos)
{
  if (colon_pos != std::string::npos) {
    std::string header_name = current_line.substr(0, colon_pos);
    std::string header_value = current_line.substr(colon_pos + 1);

    if (!header_name.empty() && !header_value.empty() && headers.find(header_name) == headers.end()) {
      headers[header_name] = header_value;
      return true;
    }
  }
  return false;
}