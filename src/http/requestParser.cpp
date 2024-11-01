#include "requestParser.hpp"

// Read a line from the raw request
std::string RequestParser::readLine(const std::string &raw_request, size_t &position) {
  
  std::cout << "Position: " << position << std::endl;
  size_t line_end = raw_request.find("\r\n", position); // start searching at position
  std::string line;
  std::cout << "line_end: " << line_end << std::endl;
  if (line_end != std::string::npos) { // \r\n found
    line = raw_request.substr(position, line_end - position);
    std::cout << "Line: " << line << std::endl;
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

bool RequestParser::parseRawRequest(HttpRequest &request, const std::string &raw_request, size_t &position) { 
    std::cout << "\n=== Parse Request Debug ===" << std::endl;
    std::cout << "Raw request to parse:\n" << raw_request << std::endl;
    std::cout << "Starting position: " << position << std::endl;
    
    try {
        // Parse the first line manually
        size_t firstLineEnd = raw_request.find("\r\n");
        if (firstLineEnd == std::string::npos) {
            throw std::runtime_error("Invalid request format - no first line");
        }
        
        std::string firstLine = raw_request.substr(0, firstLineEnd);
        std::cout << "First line: '" << firstLine << "'" << std::endl;
        
        // Split the first line into method, URI, and version
        size_t first_space = firstLine.find(' ');
        size_t second_space = firstLine.find(' ', first_space + 1);
        
        if (first_space == std::string::npos || second_space == std::string::npos) {
            throw std::runtime_error("Invalid request line format");
        }
        
        request.method = firstLine.substr(0, first_space);
        request.uri = firstLine.substr(first_space + 1, second_space - first_space - 1);
        request.version = firstLine.substr(second_space + 1);
        
        std::cout << "Parsed request line:" << std::endl;
        std::cout << "Method: '" << request.method << "'" << std::endl;
        std::cout << "URI: '" << request.uri << "'" << std::endl;
        std::cout << "Version: '" << request.version << "'" << std::endl;
        
        // Move position past the first line
        position = firstLineEnd + 2;
        
        // Parse headers
        std::cout << "\nParsing headers..." << std::endl;
        while (position < raw_request.length()) {
            size_t lineEnd = raw_request.find("\r\n", position);
            if (lineEnd == std::string::npos) {
                break;
            }
            
            std::string line = raw_request.substr(position, lineEnd - position);
            if (line.empty()) {
                break;  // Empty line marks end of headers
            }
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string name = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                // Trim leading spaces from value
                value = value.substr(value.find_first_not_of(" "));
                request.headers[name] = value;
                std::cout << "Header: '" << name << "' = '" << value << "'" << std::endl;
            }
            
            position = lineEnd + 2;
        }
        
        request.headers_parsed = true;
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Error in parseRawRequest: " << e.what() << std::endl;
        throw;
    }
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
    std::cout << "No body expected" << std::endl;
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
      std::cout << "Reading chunk size" << std::endl;
      std::string chunk_size_str = RequestParser::readLine(raw_request, position);
      if (chunk_size_str.empty()) { // should only happen in 1st iteration (1st chunk size not yet received)
        return false; // Keep reading (incomplete chunk size) -> avoid infinite loop with timeout in recv loop!
      }

      std::istringstream(chunk_size_str) >> std::hex >> request.chunk_state.chunk_size;

      std::cout << "Chunk size: " << request.chunk_state.chunk_size << std::endl;
    
      if (request.chunk_state.chunk_size == 0) {
        request.chunk_state.chunked_done = true;
        std::cout << "Chunked transfer complete" << std::endl;
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

    std::cout << "\nAvailable data: " << available_data_to_be_read << std::endl;
    std::cout << "Remaining data in chunk: " << remaining_data_in_chunk << std::endl;

    // Append only the portion of the chunk that is available in the raw_request
    size_t bytes_to_append = std::min(available_data_to_be_read, remaining_data_in_chunk);

    std::cout << "Bytes to append: " << bytes_to_append << std::endl;

    request.body.append(raw_request, position, bytes_to_append);

    std::cout << "Body: " << request.body << std::endl;

    request.chunk_state.bytes_read += bytes_to_append;

    std::cout << "Bytes read: " << request.chunk_state.bytes_read << std::endl;

    position += bytes_to_append;

    std::cout << "Position: " << position << std::endl;

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
void RequestParser::tokenizeHeaders(HttpRequest &request, const std::string &raw_request, size_t &position)
{
    std::cout << "Starting header parsing from position: " << position << std::endl;
    std::string current_line;
    
    while ((current_line = readLine(raw_request, position)) != "") {
        std::cout << "Parsing header line: '" << current_line << "'" << std::endl;
        size_t colon_pos = current_line.find(":");
        if (colon_pos != std::string::npos) {
            std::string name = current_line.substr(0, colon_pos);
            std::string value = current_line.substr(colon_pos + 1);
            // Trim leading/trailing whitespace
            value = value.substr(value.find_first_not_of(" "));
            request.headers[name] = value;
            std::cout << "Added header: '" << name << "' = '" << value << "'" << std::endl;
        }
    }
}

// Save headers in a map avoiding duplicates
bool RequestParser::validHeaderFormat(std::map<std::string, std::string> &headers, const std::string &current_line, size_t colon_pos)
{
  if (colon_pos != std::string::npos) {
    std::string header_name = current_line.substr(0, colon_pos);
    std::string header_value = current_line.substr(colon_pos + 2);

    //debugging print
    std::cout << "header name - value:" << header_name << " - " << header_value << std::endl;

    if (!header_name.empty() && !header_value.empty() && (headers.find(header_name) == headers.end()
      || header_name == "Cookie" || header_name == "Set-Cookie")) {
      headers[header_name] = header_value;
      std::cout << "header added, returning true" << std::endl;
      return true;
    }
  }
  std::cout << "header not added, returning false" << std::endl;
  return false;
}

// Extract method, URI, and version from the request line
void RequestParser::tokenizeRequestLine(HttpRequest &request, const std::string &raw_request, size_t &position) {
    std::cout << "tokenizeRequestLine starting at position: " << position << std::endl;
    
    std::string current_line = readLine(raw_request, position);
    std::cout << "Read line: '" << current_line << "'" << std::endl;
    
    if (current_line.empty()) {
        request.error_code = 400; //400 BAD_REQUEST
        throw std::runtime_error("Empty request line");
    }
    
    std::istringstream stream(current_line);
    stream >> request.method >> request.uri >> request.version;
    
    std::cout << "Parsed values:" << std::endl;
    std::cout << "Method: '" << request.method << "'" << std::endl;
    std::cout << "URI: '" << request.uri << "'" << std::endl;
    std::cout << "Version: '" << request.version << "'" << std::endl;
    
    if (stream.fail() || !validRequestLine(request)) {
        request.error_code = 400;
        throw std::runtime_error("Invalid request line format");
    }

    // Extract query string from URI
    size_t questionMarkPos = request.uri.find('?');
    if (questionMarkPos != std::string::npos) {
        request.queryString = request.uri.substr(questionMarkPos + 1);
        request.uri = request.uri.substr(0, questionMarkPos);
        std::cout << "Extracted query string: '" << request.queryString << "'" << std::endl;
    } else {
        request.queryString = "";
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
