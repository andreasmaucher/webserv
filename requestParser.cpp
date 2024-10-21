#include "requestParser.hpp"

// Read a line from the raw request
std::string RequestParser::readLine(const std::string &raw_request, size_t &position) {
  
  size_t line_end = raw_request.find("\r\n", position); // start searching at position
  std::string line;
  if (line_end != std::string::npos) { // \r\n found
    line = raw_request.substr(position, line_end - position);
    position = line_end + 2; // Move past the '\r\n' so line is pointing to the next line or the end
  }
  else {
    std::cout << "Line incomplete.\nHint: Is buffer size enough? Or chunked transfer encoding used?" << std::endl;
  }
  return line;
}

bool RequestParser::isBodyExpected(HttpRequest &request) {
    return ((request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked") ||
                         (request.headers.find("Content-Length") != request.headers.end()));
}

bool RequestParser::parseRawRequest(HttpRequest &request, const std::string &raw_request, size_t &position) { 
  
  try {
    // parse until headers only in 1st iteration
    if (request.headers_parsed == false) {
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

    return RequestParser::parseBody(request, raw_request, position);

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "HTTP Error Code: " << request.error_code << std::endl;

  }

  return true; // request complete or error. Stop reading. Handle error code in calling func to create appropriate response & clean resources
}

bool RequestParser::mandatoryHeadersPresent(HttpRequest &request) {

    // search for in HTTP/1.1 mandatory Host header presence
  if (request.headers.empty() || request.headers.find("Host") == request.headers.end()) {
    return false;
  }

  // if (request.method == "POST" && (request.headers.find("Content-Type") == request.headers.end() || ((request.headers.find("Content-Length") == request.headers.end()) || request.headers.find("Transfer-Encoding") == request.headers.end() || request.headers["Transfer-Encoding"] != "chunked"))) {
  //   return false;
  // }
  if (request.method == "POST") {
    // Check for the Content-Type header
    if (request.headers.find("Content-Type") == request.headers.end()) {
        request.error_code = 400; // Bad Request
        return false;
    }

    // Check for Content-Length or Transfer-Encoding: chunked
    bool has_content_length = request.headers.find("Content-Length") != request.headers.end();
    bool has_transfer_encoding_chunked = request.headers.find("Transfer-Encoding") != request.headers.end() && 
                                         request.headers["Transfer-Encoding"] == "chunked";

    if (!has_content_length && !has_transfer_encoding_chunked) {
        request.error_code = 411; // Length Required or 400 depending on error
        return false;
    }
}

  return true;

}

// define MAX_BODY_SIZE in requestParser.hpp or in config file? 1MB in NGINX
bool RequestParser::parseBody(HttpRequest &request, const std::string &raw_request, size_t &position) {

  // No body expected
  if (!RequestParser::isBodyExpected(request)) {
    // No body expected but there is extra data in raw_request
    if (position < raw_request.size()) {
      request.error_code = 400; // 400 BAD_REQUEST
      throw std::runtime_error("Body present but not expected");
    }
    return true; // Request complete
  }

  // Handle chunked transfer encoding
  if (request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked") {
    return saveChunkedBody(request, raw_request, position);
  }

  // Handle Content-Length body
  return saveContentLengthBody(request, raw_request, position);
}

bool RequestParser::saveChunkedBody(HttpRequest &request, const std::string &raw_request, size_t &position) {
// If we're not in the middle of a chunk, read the chunk size
    if (!request.chunk_state.in_chunk) {
      std::string chunk_size_str = RequestParser::readLine(raw_request, position);
      if (chunk_size_str.empty()) { // should only happen in 1st iteration (1st chunk size not yet received)
        return false; // Keep reading (incomplete chunk size) -> avoid infinite loop with timeout in recv loop!
      }

      std::istringstream(chunk_size_str) >> std::hex >> request.chunk_state.chunk_size;

      if (request.chunk_state.chunk_size == 0) {
        request.chunk_state.chunked_done = true;
        return true; // End of chunked transfer
      }

      if (request.chunk_state.chunk_size > MAX_BODY_SIZE || request.body.size() + request.chunk_state.chunk_size > MAX_BODY_SIZE) {
        request.error_code = 413; // 413 PAYLOAD_TOO_LARGE
        throw std::runtime_error("Body too large");
      }

      request.chunk_state.bytes_read = 0;
      request.chunk_state.in_chunk = true; // Mark that we are now inside a chunk
    }

    // Read the actual chunk data
    size_t available_data_to_be_read = raw_request.size() - position;
    size_t remaining_data_in_chunk = request.chunk_state.chunk_size - request.chunk_state.bytes_read;

    // Append only the portion of the chunk that is available in the raw_request
    size_t bytes_to_append = std::min(available_data_to_be_read, remaining_data_in_chunk);
    request.body.append(raw_request, position, bytes_to_append);
    request.chunk_state.bytes_read += bytes_to_append;
    position += bytes_to_append;

    // If we have fully read the chunk, move past the trailing \r\n and reset the chunk state
    if (request.chunk_state.bytes_read == request.chunk_state.chunk_size) {
      if (position + 2 <= raw_request.size() && raw_request.substr(position, 2) == "\r\n") {
        position += 2; // Move past \r\n
        request.chunk_state.in_chunk = false; // Chunk is fully processed, reset for the next chunk
      } else {
        request.error_code = 400; // 400 BAD_REQUEST
        throw std::runtime_error("Malformed chunked body (missing \r\n after chunk)");
      }
    }

  return false; // Keep reading for the next chunk or to finish the current chunk
}

// Handle non-chunked body when Content-Length header present
// the body length should match the value of the header
bool RequestParser::saveContentLengthBody(HttpRequest &request, const std::string &raw_request, size_t &position) {
  
  size_t content_length;
  std::istringstream(request.headers["Content-Length"]) >> content_length;

  if (content_length > MAX_BODY_SIZE || (request.body.size() + (raw_request.size() - position) > content_length)) {
    request.error_code = 413; // 413 PAYLOAD_TOO_LARGE
    throw std::runtime_error("Body too large");
  }

  // size_t bytes_to_append = std::min(content_length - request.body.size(), raw_request.size() - position);
  // request.body.append(raw_request, position, bytes_to_append);
  // position += bytes_to_append;

  // return request.body.size() == content_length; // Return true if body is complete

  if (position + content_length > raw_request.size()) {
    return false; // Keep reading (incomplete body data)
  }

  // Append the full body
  request.body.append(raw_request, position, content_length);
  position += content_length + 2; // Move past the body and the trailing \r\n

  //if there is still content after the body, it is an error
  if (position < raw_request.size()) {
    request.error_code = 400; // 400 BAD_REQUEST
    throw std::runtime_error("Extra data after body");
  }

  return true; // Full body received
  
}

// Extract headers from request until blank line (\r\n)
void RequestParser::tokenizeHeaders(HttpRequest &request, const std::string &raw_request, size_t &position) {
  
  std::string current_line = readLine(raw_request, position);
  
  if (current_line.empty()){
    request.error_code = 400; //400 BAD_REQUEST
    throw std::runtime_error("Empty headers");
    //return;
  }

  while (current_line != "\r\n") {
    size_t colon_pos = current_line.find(":");
    
    if (!validHeaderFormat(request.headers, current_line, colon_pos)) {
      request.error_code = 400; //400 BAD_REQUEST
      throw std::runtime_error("Bad header format"); // bad formatted header (twice same name, no ":")
      //return;
    }
    current_line = readLine(raw_request, position);
  }

  if (!RequestParser::mandatoryHeadersPresent(request)) {
    request.error_code = 400; //400 BAD_REQUEST
    throw std::runtime_error("Mandatory headers missing");
    //return;
  }

  request.headers_parsed = true;
}

// Save headers in a map avoiding duplicates
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
