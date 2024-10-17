#include "requestParser.hpp"

// Read a line from the raw request
std::string RequestParser::readLine(const std::string &raw_request, size_t &position) {
  
  size_t line_end = raw_request.find("\r\n", position);
  if (line_end == std::string::npos) {
    throw std::runtime_error("Invalid request line (line too long).\nHint: Is buffer size enough?");
  }
  std::string line = raw_request.substr(position, line_end - position);
  position = line_end + 2; // Move past the '\r\n'
  return line;
}

bool RequestParser::parseRawRequest(HttpRequest &request, const std::string &raw_request, size_t &position) { 
  
  try {

    if (request.method.empty()) { // only in 1st iteration

      RequestParser::tokenizeRequestLine(request, raw_request, position);
      //debugging print
      std::cout << "Method: " << request.method << "\n"
                << "URI: " << request.uri << "\n"
                << "Version: " << request.version << std::endl;

      RequestParser::tokenizeHeaders(request, raw_request, position);
      //debugging print
      std::cout << "Headers:\n";
      for (std::map<std::string, std::string>::iterator header_iter = request.headers.begin(); header_iter != request.headers.end(); ++header_iter) {
        std::cout << "    " << header_iter->first << " " << header_iter->second << std::endl;
      }
    }

    // at this point:
    // we need to check if there is a body expected, and if so, if there is a part already save it,
    // if there is no body expected, but there is a part -> bad request
    // if there is no body expected and no part -> return to calling function and signalize that the request is complete
    // if there is a body expected and no part -> keep reading until the body is complete
    // if there is a body expected and a part -> save the part and return to calling function to keep reading until the body is complete
  
    if (!RequestParser::saveValidBody(request, raw_request, position)) {
      return true; // no body expected, request complete
    }
    //debugging print
    std::cout << "Body: " << request.body << std::endl;
    
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "HTTP Error Code: " << request.error_code << std::endl;

    return true; // handle error code in calling func to create appropriate response & clean resources

  }

  return false; // keep reading request
}

bool RequestParser::saveValidBody(HttpRequest &request, const std::string &raw_request, size_t &position) {

  if ((++it)->empty() || it == request_lines.end())
    return;
  // If the request has a Transfer-Encoding header with the value "chunked", the body should be read until an empty line
  if (request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked") {

    std::string chunked_body;
    // while (it != request_lines.end() && !(*it).empty()) {
    //   chunked_body += *it;
    //   ++it;
    // }
    while (true) {
      // Read the chunk size
      std::string chunk_size_str = *it;
      std::istringstream iss(chunk_size_str);
      size_t chunk_size;
      iss >> std::hex >> chunk_size; // Convert hex string to decimal

      ++it; // Move to the chunk data

      if (chunk_size == 0) {
        // End of chunked transfer (chunk size 0)
        break;
      }

      // Append chunk data to body
      chunked_body.append(*it, 0, chunk_size);

      ++it; // Move past the current chunk
    }

    request.body = chunked_body;

  }
  // If the request has a Content-Length header, the body length should match the value of the header
  else if(request.headers.find("Content-Length") != request.headers.end()) {
      
    std::istringstream iss(request.headers["Content-Length"]);
    size_t content_length;
    iss >> content_length; // Get the integer value of content length
    
    if (content_length != (*it).size()) {
      request.error_code = 400; //400 BAD_REQUEST
      throw std::runtime_error("Invalid body length");
      //return false;
    }
    
    request.body = *it;

  }
}

// Extract headers from request until blank line (\r\n)
void RequestParser::tokenizeHeaders(HttpRequest &request, const std::string &raw_request, size_t &position) {
  
  std::string current_line = readLine(raw_request, position);
  
  while (current_line != "\r\n") {
    size_t colon_pos = current_line.find(":");
    
    if (!validHeaderFormat(request.headers, current_line, colon_pos)) {
      request.error_code = 400; //400 BAD_REQUEST
      throw std::runtime_error("Bad header format"); // bad formatted header (twice same name, no ":")
      //return;
    }
    current_line = readLine(raw_request, position);
  }
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

// Extract method, URI, and version from the request line
void RequestParser::tokenizeRequestLine(HttpRequest &request, const std::string &raw_request, size_t &position) {
  
  std::string current_line = readLine(raw_request, position);
  if (current_line.empty()){
    request.error_code = 400; //400 BAD_REQUEST
    throw std::runtime_error("Empty request line");
    //return;
  }
  
  std::istringstream stream(current_line);
  
  stream >> request.method >> request.uri >> request.version;
  if (stream.fail() || !validRequestLine(request)) {
    return;
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

// add check for invalid chars?
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
