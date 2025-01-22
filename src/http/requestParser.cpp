#include "../../include/requestParser.hpp"

void RequestParser::parseRawRequest(HttpRequest &request) { 
  
  try {
    // Check if we have a complete request (headers end with \r\n\r\n)
    if (request.raw_request.find("\r\n\r\n") == std::string::npos) {
      std::cout << "Request incomplete, waiting for more data..." << std::endl;
      return; // Wait for more data
    }

    // parse until headers only in 1st iteration
    if (request.headers_parsed == false) {
      std::cout << "Complete request received, parsing..." << std::endl;
      std::cout << "Raw request:\n[" << request.raw_request << "]" << std::endl;

      RequestParser::tokenizeRequestLine(request);
      std::cout << "Request Line parsed: " << request.method << " " << request.uri << " " << request.version << std::endl;

      RequestParser::tokenizeHeaders(request);
      std::cout << "Headers parsed....................................." << std::endl;
    }

    std::cout << "Parsing body..." << std::endl;

    return RequestParser::parseBody(request);

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "HTTP Error Code: " << request.error_code << std::endl;

    request.complete = true;

    return; // error. Stop reading. Handle error code in calling func to create appropriate response & clean resources
  }

}

std::string RequestParser::readLine(const std::string &raw_request, size_t &position) {
  
  size_t line_end = raw_request.find("\r\n", position);
  std::string line;
  if (line_end != std::string::npos) { // \r\n found
    line = raw_request.substr(position, line_end - position);
    position = line_end + 2; // Move past the '\r\n' so line is pointing to the next line or the end
  }
  else {
    std::cout << "Line incomplete.\nHint: Is buffer size enough? Or chunked transfer encoding used? -> make sure to read until end of headers before starting to parse" << std::endl;
  }
  return line;
}

bool RequestParser::isBodyExpected(HttpRequest &request) {
    return ((request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked")
      || (request.headers.find("Content-Length") != request.headers.end()));
}

bool RequestParser::mandatoryHeadersPresent(HttpRequest &request) {

  // search for in HTTP/1.1 mandatory Host header presence
  if (request.headers.empty() || request.headers.find("Host") == request.headers.end()) {
    return false;
  }

  if (request.method == "POST") {
    // Check for the Content-Type header
    // Check for Content-Length or Transfer-Encoding: chunked
    bool has_content_length = request.headers.find("Content-Length") != request.headers.end();
    bool has_transfer_encoding_chunked = request.headers.find("Transfer-Encoding") != request.headers.end() && 
                                         request.headers["Transfer-Encoding"] == "chunked";
    if (!has_content_length && !has_transfer_encoding_chunked) {
        request.error_code = 411;
        return false;
    }
}

  return true;

}

// define MAX_BODY_SIZE in requestParser.hpp or in config file? 1MB in NGINX
void RequestParser::parseBody(HttpRequest &request) {

  if (!RequestParser::isBodyExpected(request)) {
    std::cout << "No body expected" << std::endl;
    // No body expected but there is extra data in raw_request
    if (request.position < request.raw_request.size()) {
      request.error_code = 400;
      throw std::runtime_error("Body present but not expected");
    }
    request.complete = true;
    return ;
  }

  if (request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked") {
    return saveChunkedBody(request);
  }

  return saveContentLengthBody(request);
}

void RequestParser::saveChunkedBody(HttpRequest &request) {

    // If we're not in the middle of a chunk, read the chunk size
    if (!request.chunk_state.in_chunk) {
      std::string chunk_size_str = RequestParser::readLine(request.raw_request, request.position);
      if (chunk_size_str.empty()) {
        return ; // Keep reading (incomplete chunk size) -> avoid infinite loop with timeout in recv loop!
      }
      std::istringstream(chunk_size_str) >> std::hex >> request.chunk_state.chunk_size;
    
      if (request.chunk_state.chunk_size == 0) {
        request.chunk_state.chunked_done = true;
        std::cout << "Chunked transfer complete" << std::endl;
        request.complete = true;
        return ; // End of chunked transfer
      }
      if (request.chunk_state.chunk_size > MAX_BODY_SIZE || request.body.size() + request.chunk_state.chunk_size > MAX_BODY_SIZE) {
        request.error_code = 413;
        throw std::runtime_error("Body too large");
      }
      request.chunk_state.bytes_read = 0;
      request.chunk_state.in_chunk = true;
    }

    // Read the actual chunk data
    size_t available_data_to_be_read = request.raw_request.size() - request.position;
    size_t remaining_data_in_chunk = request.chunk_state.chunk_size - request.chunk_state.bytes_read;

    // Append only the portion of the chunk that is available in the raw_request
    size_t bytes_to_append = std::min(available_data_to_be_read, remaining_data_in_chunk);
    request.body.append(request.raw_request, request.position, bytes_to_append);
    request.chunk_state.bytes_read += bytes_to_append;
    request.position += bytes_to_append;

    // If we have fully read the chunk, move past the trailing \r\n and reset the chunk state
    if (request.chunk_state.bytes_read == request.chunk_state.chunk_size) {
      if (request.position + 2 <= request.raw_request.size() && request.raw_request.substr(request.position, 2) == "\r\n") {
        request.position += 2; // Move past \r\n
        request.chunk_state.in_chunk = false; // Chunk is fully processed, reset for the next chunk
      } else {
        request.error_code = 400;
        throw std::runtime_error("Malformed chunked body (missing \r\n after chunk)");
      }
    }
  return; // Keep reading for the next chunk or to finish the current chunk
}

// Handle non-chunked body when Content-Length header present
// the body length should match the value of the header
void RequestParser::saveContentLengthBody(HttpRequest &request) {
  
  size_t content_length;
  std::istringstream(request.headers["Content-Length"]) >> content_length;
  size_t bytes_needed = content_length - request.body.size();

  // If the content_length is too large or the position is already beyond the total raw request size
  if (content_length > MAX_BODY_SIZE || request.position >= request.raw_request.size()) {
      request.error_code = 413;
      throw std::runtime_error("Body too large or incomplete");
  }

  // Determine how many bytes we can read
  size_t bytes_to_append = std::min(bytes_needed, request.raw_request.size() - request.position);
  request.body.append(request.raw_request, request.position, bytes_to_append);
  request.position += bytes_to_append;

  // If we already have the complete body, check for extra data
  if (request.body.size() == content_length) {
      if (request.position < request.raw_request.size()) {
          request.error_code = 400; // 400 BAD_REQUEST
          throw std::runtime_error("Extra data after body");
      }
      std::cout << "Body complete" << std::endl;
      request.complete = true;
      return; // Full body received
  }
}

// Extract headers from request until blank line (\r\n)
void RequestParser::tokenizeHeaders(HttpRequest &request) {
  
  std::string current_line = readLine(request.raw_request, request.position);
  
  if (current_line.empty()){
    request.error_code = 400;
    throw std::runtime_error("Empty headers");
  }

  while (!current_line.empty()) {
    size_t colon_pos = current_line.find(":");
    
    if (!validHeaderFormat(request.headers, current_line, colon_pos)) {
      request.error_code = 400;
      throw std::runtime_error("Bad header format"); // bad formatted header (twice same name, no ":")
    }
    current_line = readLine(request.raw_request, request.position);
  }

  if (!RequestParser::mandatoryHeadersPresent(request)) {
    request.error_code = 400;
    throw std::runtime_error("Mandatory headers missing");
  }

  request.headers_parsed = true;
}

// Save headers in a map avoiding duplicates
bool RequestParser::validHeaderFormat(std::map<std::string, std::string> &headers, const std::string &current_line, size_t colon_pos)
{
  if (colon_pos != std::string::npos) {
    std::string header_name = current_line.substr(0, colon_pos);
    std::string header_value = current_line.substr(colon_pos + 2);

    if (!header_name.empty() && !header_value.empty() && (headers.find(header_name) == headers.end()
      || header_name == "Cookie" || header_name == "Set-Cookie")) {
      headers[header_name] = header_value;
      return true;
    }
  }
  return false;
}

// Extract method, URI, and version from the request line
void RequestParser::tokenizeRequestLine(HttpRequest &request) {
  
  std::string current_line = readLine(request.raw_request, request.position);
  if (current_line.empty()){
    request.error_code = 400;
    throw std::runtime_error("Empty request line");
  }
  
  std::istringstream stream(current_line);
  
  stream >> request.method >> request.uri >> request.version;
  if (stream.fail() || !validRequestLine(request)) {
    request.error_code = 400;
    throw std::runtime_error("Bad request line");
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
  request.error_code = 405;
  throw std::runtime_error("Invalid method");

}

// add check for invalid chars?
bool RequestParser::validPathFormat(HttpRequest &request)
{
  if (request.uri.empty() || request.uri[0] != '/' || request.uri.find(" ") != std::string::npos)
  {
    request.error_code = 400;
    throw std::runtime_error("Invalid path format");
  }
  return true;
}

bool RequestParser::validHttpVersion(HttpRequest &request)
{
  if (!request.version.empty() && request.version == "HTTP/1.1")
  {
    return true;
  }
  request.error_code = 505;
  throw std::runtime_error("Invalid HTTP version");
}