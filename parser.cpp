#include "parser.hpp"

// Split the raw request into lines (raw_request = buffer passed to
// recv())
std::vector<std::string> parser::splitIntoLines(const std::string &raw_request) {
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
// to add: check for like bad formatted or empty field(s)
void parser::tokenizeRequestLine(const std::string &requestLine, httpRequest &request) {
  if (requestLine.empty())
    return;
  
  std::istringstream stream(requestLine);
  
  stream >> request.method >> request.uri >> request.version;
}

// Extract headers from request until blank line (\r\n)
// to add: edge cases like twice same header name, check for wrong formatted
// header(s)...
void parser::tokenizeHeaders(std::vector<std::string>::iterator &it, const std::vector<std::string> &requestLines,  httpRequest &request) {
  
  bool endOfHeaders = false;
  if (it->empty())
    return;

  while (++it != requestLines.end() && !it->empty() && !endOfHeaders) {
    std::string headerLine = *it;
    size_t colonPos = headerLine.find(":");
    
    if (colonPos != std::string::npos) {
      std::string headerName = headerLine.substr(0, colonPos);
      std::string headerValue = headerLine.substr(colonPos + 1);

      request.headers[headerName] = headerValue;
    
    } else { // temporary solution for bad formatted & blank line
      endOfHeaders = true;
    }
  }
}


void parser::saveBody(std::vector<std::string>::iterator &it, const std::vector<std::string> &requestLines,  httpRequest &request) {

  if (!(++it)->empty() && it != requestLines.end() )
   request.body = *it;
}