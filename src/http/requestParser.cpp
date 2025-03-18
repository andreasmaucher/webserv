#include "../../include/requestParser.hpp"
#include "../../include/debug.hpp"

// this function determines if the request is for a directory and sets the is_directory flag accordingly
void RequestParser::checkForDirectory(HttpRequest &request)
{
    request.is_directory = false;
    std::string fullPath;
    if (request.route) {
        fullPath = request.route->path + request.uri;
    } else {
        fullPath = "./www" + request.uri;
    }
    
    // Check if the path exists
    struct stat path_stat;
    if (stat(fullPath.c_str(), &path_stat) != 0) {
        return;
    }
    
    // Check if it's actually a directory
    if (S_ISDIR(path_stat.st_mode) ) 
    {
        request.is_directory = true;
        
        if (!request.uri.empty() && request.uri[request.uri.length() - 1] != '/') {
            request.uri += '/';
        }
        return;
    }
    
    // Handle the edge case of URIs that end with a slash but are files
    if (!request.uri.empty() && request.uri[request.uri.length() - 1] == '/' && !request.is_directory) {
        return;
    }
}

void RequestParser::parseRawRequest(HttpRequest &request)
{
  std::cout << "beginning function parseRawRequest" << std::endl;
  try
  {
    // Check if we have a complete request (headers end with \r\n\r\n)
    if (request.raw_request.find("\r\n\r\n") == std::string::npos)
    {
      std::cout << "request incomplete, waiting for more data..." << std::endl;
      DEBUG_MSG("Request incomplete, waiting for more data...", "");
      return; // Wait for more data
    }

    // parse until headers only in 1st iteration
    if (request.headers_parsed == false)
    {
      std::cout << "headers_parsed is false" << std::endl;
      DEBUG_MSG("Complete request received, parsing...", +"");
      DEBUG_MSG("Raw request:\n[", request.raw_request);
      DEBUG_MSG("]", "");

      RequestParser::tokenizeRequestLine(request);
      DEBUG_MSG("Request Line parsed: request.method ", request.method);
      DEBUG_MSG("Request.version ", request.version);

      RequestParser::checkForDirectory(request);

      RequestParser::tokenizeHeaders(request);
      DEBUG_MSG("Headers parsed.....................................", "");
    }

    DEBUG_MSG("Parsing body...", "");
    std::cout << "before parseBody called" << std::endl;
    return RequestParser::parseBody(request);
  }
  catch (std::exception &e)
  {
    std::cout << "error in parseRawRequest" << std::endl;
    DEBUG_MSG("Error: ", e.what());
    DEBUG_MSG("HTTP Error Code: ", request.error_code);

    request.complete = true;
    return;
  }
}

std::string RequestParser::readLine(const std::string &raw_request, size_t &position)
{

  size_t line_end = raw_request.find("\r\n", position);
  std::string line;
  if (line_end != std::string::npos)
  { // \r\n found
    line = raw_request.substr(position, line_end - position);
    position = line_end + 2; // Move past the '\r\n' so line is pointing to the next line or the end
  }
  else
  {
    DEBUG_MSG("Line incomplete.\nHint: Is buffer size enough? Or chunked transfer encoding used? -> make sure to read until end of headers before starting to parse", "");
  }
  return line;
}

bool RequestParser::isBodyExpected(HttpRequest &request)
{
  return ((request.headers.find("Transfer-Encoding") != request.headers.end() && request.headers["Transfer-Encoding"] == "chunked") || (request.headers.find("Content-Length") != request.headers.end()));
}

bool RequestParser::mandatoryHeadersPresent(HttpRequest &request)
{

  // search for in HTTP/1.1 mandatory Host header presence
  if (request.headers.empty() || request.headers.find("Host") == request.headers.end())
  {
    return false;
  }
  // MICHAEL : Need to add GET and DELETE
  if (request.method == "POST")
  {
    // Check for the Content-Type header
    // Check for Content-Length or Transfer-Encoding: chunked
    bool has_content_length = request.headers.find("Content-Length") != request.headers.end();
    bool has_transfer_encoding_chunked = request.headers.find("Transfer-Encoding") != request.headers.end() &&
                                         request.headers["Transfer-Encoding"] == "chunked";
    if (!has_content_length && !has_transfer_encoding_chunked)
    {
      request.error_code = 411;
      return false;
    }
  }
  else if (request.method == "GET")
  {
  }
  else if (request.method == "DELETE")
  {
  }
  else
  {
    DEBUG_MSG("-------->Weird allow method: ", request.method);
    request.error_code = 406;
    return false;
  }

  return true;
}

// Add this new function after the parseBody function
bool RequestParser::isMultipartRequestComplete(const HttpRequest &request)
{
    std::cout << "Start Multipart Request Function" << std::endl;
    // First check if it's a multipart request
    std::map<std::string, std::string>::const_iterator it = request.headers.find("Content-Type");
    if (it == request.headers.end() || it->second.find("multipart/form-data") == std::string::npos) {
        // Not a multipart request
        return false;
    }
    
    // Extract the boundary
    size_t boundaryPos = it->second.find("boundary=");
    if (boundaryPos == std::string::npos) {
        std::cout << "No boundary found in Content-Type: " << it->second << std::endl;
        return false;
    }
    
    std::string boundary = it->second.substr(boundaryPos + 9);
    // Remove quotes if present
    if (!boundary.empty() && boundary[0] == '"') {
        size_t endQuote = boundary.find('"', 1);
        if (endQuote != std::string::npos) {
            boundary = boundary.substr(1, endQuote - 1);
        }
    }
    
    // Get Content-Length if available
    size_t contentLength = 0;
    std::map<std::string, std::string>::const_iterator cl_it = request.headers.find("Content-Length");
    if (cl_it != request.headers.end()) {
        std::istringstream(cl_it->second) >> contentLength;
    }
    
    static int checkCount = 0;
    static std::string lastRequestId;
    
    // Create a unique ID for this request to track retries
    std::string currentRequestId = request.raw_request.substr(0, std::min(30, (int)request.raw_request.size()));
    if (lastRequestId != currentRequestId) {
        checkCount = 0;
        lastRequestId = currentRequestId;
    }
    
    checkCount++;
    
    std::cout << "Checking for final boundary variants" << std::endl;
    std::cout << "Check #" << checkCount << ": Body size=" << request.body.size() 
              << ", Content-Length=" << contentLength << std::endl;
    
    // Check for the end boundary in different formats
    std::string endBoundary1 = "--" + boundary + "--";
    std::string endBoundary2 = "\r\n--" + boundary + "--";
    std::string endBoundary3 = "\n--" + boundary + "--";
    
    if (request.body.find(endBoundary1) != std::string::npos ||
        request.body.find(endBoundary2) != std::string::npos ||
        request.body.find(endBoundary3) != std::string::npos) {
        std::cout << "Final boundary found" << std::endl;
        checkCount = 0;
        return true;
    }
    
    std::cout << "Final boundary not found" << std::endl;
    
    // IMPORTANT: Only force completion when we have nearly all the data
    if (contentLength > 0) {
        double percentReceived = (static_cast<double>(request.body.size()) / contentLength) * 100.0;
        std::cout << "Received " << percentReceived << "% of expected data" << std::endl;
        
        // We consider the request complete only if we've received at least 95% of the data
        if (percentReceived >= 95.0) {
            std::cout << "Received at least 95% of Content-Length, forcing completion" << std::endl;
            checkCount = 0;
            return true;
        }
    }
    
    // Additional safety limit - if we've run for too many iterations AND received a significant portion
    if (checkCount > 100 && contentLength > 0) {
        double percentReceived = (static_cast<double>(request.body.size()) / contentLength) * 100.0;
        if (percentReceived >= 90.0) {
            std::cout << "Check count exceeded 100 with 90% data, forcing completion" << std::endl;
            checkCount = 0;
            return true;
        }
    }
    
    // Extreme safety valve - prevent infinite loops for malformed requests
    // but ONLY after we've received at least 50% of the data
    if (checkCount > 200 && contentLength > 0) {
        double percentReceived = (static_cast<double>(request.body.size()) / contentLength) * 100.0;
        if (percentReceived >= 50.0) {
            std::cout << "Emergency completion after 200 checks with 50% data" << std::endl;
            checkCount = 0;
            return true;
        }
    }
    
    // Print detailed info for debugging
    std::cout << "multipart body incomplete 2" << std::endl;
    std::cout << "End Multipart Request Function" << std::endl;
    return false;
}

// define MAX_BODY_SIZE in requestParser.hpp or in config file? 1MB in NGINX
void RequestParser::parseBody(HttpRequest &request)
{
  DEBUG_MSG("Parsing body...", "");
    // If we expect a body (based on headers)
    std::cout << "parsing body" << std::endl;
    if (isBodyExpected(request))
    {
        std::cout << "isBodyExpected true within parseBody" << std::endl;
        size_t content_length = 0;
        std::istringstream(request.headers["Content-Length"]) >> content_length;
        
        DEBUG_MSG("Expected Content-Length", content_length);
        DEBUG_MSG("Current body size", request.body.size());
        
        // Calculate remaining data after headers
        size_t headers_end = request.raw_request.find("\r\n\r\n");
        if (headers_end != std::string::npos)
        {
            headers_end += 4; // Move past \r\n\r\n
            std::string body_data = request.raw_request.substr(headers_end);
            request.body = body_data;
            
            // Special handling for multipart/form-data (used by file uploads)
            std::map<std::string, std::string>::const_iterator it = request.headers.find("Content-Type");
            if (it != request.headers.end() && it->second.find("multipart/form-data") != std::string::npos) {
                DEBUG_MSG("Multipart form data detected", it->second);
                
                // Check if we have received the final boundary
                if (isMultipartRequestComplete(request)) {
                    request.complete = true;
                    std::cout << "multipart body complete" << std::endl;
                    DEBUG_MSG("Multipart request complete", "");
                }else {
                    DEBUG_MSG("Multipart request incomplete", "Waiting for final boundary");
                    std::cout << "multipart body incomplete in parseBody" << std::endl;
                    request.complete = false;
                }
                return; // Return early for multipart requests
            }
            
            // Normal Content-Length check for non-multipart requests
            if (request.body.length() >= content_length) // Changed from == to >=
            {
                request.complete = true;
                std::cout << "body complete" << std::endl;
                DEBUG_MSG("Body complete", "");
            }
            else
            {
              DEBUG_MSG("Body incomplete", "Waiting for more data");
                std::cout << "body incomplete" << std::endl;
                request.complete = false;
            }
        }
    }
    else
    {
        request.complete = true;
        DEBUG_MSG("No body expected", "");
    }
  //! ANDY edit to allow .png uploads
    /* DEBUG_MSG("Parsing body...", "");
    // If we expect a body (based on headers)
    std::cout << "parsing body" << std::endl;
    if (isBodyExpected(request))
    {
        size_t content_length = 0;
        std::istringstream(request.headers["Content-Length"]) >> content_length;
        
        DEBUG_MSG("Expected Content-Length", content_length);
        DEBUG_MSG("Current body size", request.body.size());
        
        // Calculate remaining data after headers
        size_t headers_end = request.raw_request.find("\r\n\r\n");
        if (headers_end != std::string::npos)
        {
            headers_end += 4; // Move past \r\n\r\n
            std::string body_data = request.raw_request.substr(headers_end);
            request.body = body_data;
            
            // Check if we have received all the data
            if (request.body.length() == content_length)
            {
                request.complete = true;
                std::cout << "body complete" << std::endl;
                DEBUG_MSG("Body complete", "");
            }
            else
            {
                DEBUG_MSG("Body incomplete", "Waiting for more data");
                std::cout << "body incomplete" << std::endl;
                request.complete = false;
            }
        }
    }
    else
    {
        request.complete = true;
        DEBUG_MSG("No body expected", "");
    } */
}

void RequestParser::saveChunkedBody(HttpRequest &request)
{
  // If we're not in the middle of a chunk, read the chunk size
  if (!request.chunk_state.in_chunk)
  {
    std::string chunk_size_str = RequestParser::readLine(request.raw_request, request.position);
    if (chunk_size_str.empty())
    {
      return; // Keep reading (incomplete chunk size) -> avoid infinite loop with timeout in recv loop!
    }
    std::istringstream(chunk_size_str) >> std::hex >> request.chunk_state.chunk_size;

    if (request.chunk_state.chunk_size == 0)
    {
      request.chunk_state.chunked_done = true;
      DEBUG_MSG("Chunked transfer complete", "");
      request.complete = true;
      return; // End of chunked transfer
    }
    if (request.chunk_state.chunk_size > MAX_BODY_SIZE || request.body.size() + request.chunk_state.chunk_size > MAX_BODY_SIZE)
    {
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
  if (request.chunk_state.bytes_read == request.chunk_state.chunk_size)
  {
    if (request.position + 2 <= request.raw_request.size() && request.raw_request.substr(request.position, 2) == "\r\n")
    {
      request.position += 2;                // Move past \r\n
      request.chunk_state.in_chunk = false; // Chunk is fully processed, reset for the next chunk
    }
    else
    {
      request.error_code = 400;
      throw std::runtime_error("Malformed chunked body (missing \r\n after chunk)");
    }
  }
  return; // Keep reading for the next chunk or to finish the current chunk
}

// Handle non-chunked body when Content-Length header present
// the body length should match the value of the header
void RequestParser::saveContentLengthBody(HttpRequest &request)
{
    size_t content_length;
    std::istringstream(request.headers["Content-Length"]) >> content_length;
    // Check max size
    if (content_length > MAX_BODY_SIZE)
    {
        request.error_code = 413;
        throw std::runtime_error("Body too large");
    }

    // Calculate how much data we can read
    size_t remaining_bytes = content_length - request.body.size();
    size_t available_bytes = request.raw_request.size() - request.position;
    size_t bytes_to_append = std::min(remaining_bytes, available_bytes);

    if (bytes_to_append > 0)
    {
        request.body.append(request.raw_request, request.position, bytes_to_append);
        request.position += bytes_to_append;
    }
    // Check if we have the complete body
    if (request.body.size() == content_length)
    {
        request.complete = true;
        return;
    }
    else if (request.body.size() > content_length)
    {
      request.error_code = 400; // 400 BAD_REQUEST
      throw std::runtime_error("Extra data after body");
    }
    DEBUG_MSG("Body complete", "");
    request.complete = true;
    return; // Full body received
}

// Extract headers from request until blank line (\r\n)
void RequestParser::tokenizeHeaders(HttpRequest &request)
{

  std::string current_line = readLine(request.raw_request, request.position);

  if (current_line.empty())
  {
    request.error_code = 400;
    throw std::runtime_error("Empty headers");
  }

  while (!current_line.empty())
  {
    size_t colon_pos = current_line.find(":");

    if (!validHeaderFormat(request.headers, current_line, colon_pos))
    {
      request.error_code = 400;
      throw std::runtime_error("Bad header format"); // bad formatted header (twice same name, no ":")
    }
    current_line = readLine(request.raw_request, request.position);
  }

  if (!RequestParser::mandatoryHeadersPresent(request))
  {
    request.error_code = 400;
    throw std::runtime_error("Mandatory headers missing");
  }

  request.headers_parsed = true;
}

// Save headers in a map avoiding duplicates
bool RequestParser::validHeaderFormat(std::map<std::string, std::string> &headers, const std::string &current_line, size_t colon_pos)
{
  if (colon_pos != std::string::npos)
  {
    std::string header_name = current_line.substr(0, colon_pos);
    std::string header_value = current_line.substr(colon_pos + 2);

    if (!header_name.empty() && !header_value.empty() && (headers.find(header_name) == headers.end() || header_name == "Cookie" || header_name == "Set-Cookie"))
    {
      headers[header_name] = header_value;
      return true;
    }
  }
  return false;
}

// Extract method, URI, and version from the request line
void RequestParser::tokenizeRequestLine(HttpRequest &request)
{

  std::string current_line = readLine(request.raw_request, request.position);
  if (current_line.empty())
  {
    request.error_code = 400;
    throw std::runtime_error("Empty request line");
  }

  std::istringstream stream(current_line);

  stream >> request.method >> request.uri >> request.version;
  if (stream.fail() || !validRequestLine(request))
  {
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