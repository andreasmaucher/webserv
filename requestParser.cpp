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

bool RequestParser::parseRawRequest(HttpRequest &request, const std::string &raw_request, size_t &position) { 
  
  try {
    // parse until headers only in 1st iteration
    if (request.method.empty()) {
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

    bool body_expected = (request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked") ||
                         (request.headers.find("Content-Length") != request.headers.end());

    if (!body_expected) {
      // No body expected but there is extra data in raw_request
      if (position < raw_request.size()) {
        request.error_code = 400; // 400 BAD_REQUEST
        throw std::runtime_error("Body present but not expected");
      }
      return true; // Request complete (no body to read)
    }

    // If the body is expected, handle it
    return RequestParser::saveValidBody(request, raw_request, position);

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "HTTP Error Code: " << request.error_code << std::endl;

  }

  return true; // request complete or error. Stop reading. Handle error code in calling func to create appropriate response & clean resources
}

// if there is a body expected and no part -> keep reading
// if there is a body expected and a part -> save the part
// avoid keep parsing if body exceeds limits
// define MAX_BODY_SIZE in requestParser.hpp or in config file? 1MB in NGINX

struct ChunkState {
    size_t chunk_size;    // Size of the current chunk
    size_t bytes_read;    // Bytes of the current chunk that have been read
    bool in_chunk;        // Indicates whether we are in the middle of reading a chunk
    bool chunked_done;    // Indicates whether the chunked transfer is complete

    ChunkState() : chunk_size(0), bytes_read(0), in_chunk(false), chunked_done(false) {}
};

bool RequestParser::saveValidBody(HttpRequest &request, const std::string &raw_request, size_t &position, ChunkState &chunk_state) {

  // Handle chunked transfer encoding
  if (request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked") {

    while (position < raw_request.size()) {

      // If we're not in the middle of a chunk, read the chunk size
      if (!chunk_state.in_chunk) {
        std::string chunk_size_str = readLine(raw_request, position);
        if (chunk_size_str.empty()) {
          return false; // Keep reading (incomplete chunk size)
        }

        std::istringstream(chunk_size_str) >> std::hex >> chunk_state.chunk_size;

        if (chunk_state.chunk_size == 0) {
          chunk_state.chunked_done = true;
          return true; // End of chunked transfer
        }

        if (chunk_state.chunk_size > MAX_BODY_SIZE || request.body.size() + chunk_state.chunk_size > MAX_BODY_SIZE) {
          request.error_code = 413; // 413 PAYLOAD_TOO_LARGE
          throw std::runtime_error("Body too large");
        }

        chunk_state.bytes_read = 0;
        chunk_state.in_chunk = true; // Mark that we are now inside a chunk
      }

      // Now, read the chunk data
      size_t remaining_data_in_request = raw_request.size() - position;
      size_t remaining_data_in_chunk = chunk_state.chunk_size - chunk_state.bytes_read;

      if (remaining_data_in_request < remaining_data_in_chunk) {
        // Only part of the chunk is available, append what we can and wait for more data
        request.body.append(raw_request, position, remaining_data_in_request);
        chunk_state.bytes_read += remaining_data_in_request;
        position += remaining_data_in_request;
        return false; // Keep reading (incomplete chunk data)
      }

      // Full chunk data is available
      request.body.append(raw_request, position, remaining_data_in_chunk);
      position += remaining_data_in_chunk;
      chunk_state.bytes_read += remaining_data_in_chunk;

      // After reading the chunk, move past the trailing \r\n
      if (raw_request.substr(position, 2) == "\r\n") {
        position += 2;
      } else {
        request.error_code = 400; // 400 BAD_REQUEST
        throw std::runtime_error("Malformed chunked body (missing \r\n after chunk)");
      }

      // Mark that we finished reading this chunk
      chunk_state.in_chunk = false;
    }

    return false; // Keep reading for the next chunk or to finish current chunk
  }
// bool RequestParser::saveValidBody(HttpRequest &request, const std::string &raw_request, size_t &position) {
//   // if (body.empty()) { // infinite loop if no body present but expected and never received, handle with timeout in recv loop!!
//   //   return false; // keep reading
//   // }

//   // If the request has a Transfer-Encoding header with the value "chunked", the body should be read until a chunk size 0 followed by an empty line
//   if (request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked") {

//     while (true) {
//       // Read the chunk size
//       std::string chunk_size_str = readLine(raw_request, position);
//       if (chunk_size_str.empty()) {
//         return false; // Keep reading (1st chunk didn't arrive yet)
//       }

//       std::istringstream iss(chunk_size_str);
//       size_t chunk_size;
//       // Convert hex string to decimal
//       iss >> std::hex >> chunk_size;

//       // End of chunked transfer (chunk size 0)
//       if (chunk_size == 0) {
//         return true;
//       }

//       if (chunk_size > MAX_BODY_SIZE || request.body.size() + chunk_size > MAX_BODY_SIZE) {
//         request.error_code = 413; //413 PAYLOAD_TOO_LARGE
//         throw std::runtime_error("Body too large");
//         //return true;
//       }

//       // Read the chunk data
//       if (position + chunk_size > raw_request.size()) {
//         return false; // Keep reading (truncated chunk)
//       }

//       request.body.append(raw_request, position, chunk_size);
//       position += chunk_size;
     
//       // Skip the trailing \r\n after each chunk
//       if (raw_request.substr(position, 2) == "\r\n") {
//         position += 2;
//       } else {
//         request.error_code = 400; // 400 BAD_REQUEST
//         throw std::runtime_error("Malformed chunked body");
//       }
//     }
//   }

  // If the request has a Content-Length header, the body length should match the value of the header

  // Handle non-chunked body with Content-Length
  else if (request.headers.find("Content-Length") != request.headers.end()) {
    size_t content_length;
    std::istringstream(request.headers["Content-Length"]) >> content_length;

    if (content_length > MAX_BODY_SIZE) {
      request.error_code = 413; // 413 PAYLOAD_TOO_LARGE
      throw std::runtime_error("Body too large");
    }

    if (position + content_length > raw_request.size()) {
      return false; // Keep reading (incomplete body data)
    }

    // Append the full body
    request.body.append(raw_request, position, content_length);
    position += content_length;

    //if there is still content after the body, it is an error
    if (position < raw_request.size()) {
      request.error_code = 400; // 400 BAD_REQUEST
      throw std::runtime_error("Extra data after body");
    }

    return true; // Full body received
  }
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
