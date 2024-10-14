#include "parser.hpp"

// Split the raw request into lines (raw_request = buffer passed to
// recv())
std::vector<std::string>
parser::splitIntoLines(const std::string &raw_request) {
  std::istringstream stream(raw_request);
  std::string line;
  std::vector<std::string> lines;

  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back(); // Remove '\r'
    }
    lines.push_back(line);
  }
  return lines;
}

// Extract method, URI, and version from the request line
// to add: check for like bad formatted or empty field(s)
void parser::tokenizeRequestLine(const std::string &requestLine,
                                 httpRequest &request) {
  std::istringstream stream(requestLine);
  stream >> request.method >> request.uri >> request.version;
}

// Extract headers from request until blank line (\r\n)
// to add: edge cases like twice same header name, check for wrong formatted
// header(s)...
bool parser::tokenizeHeaders(const std::string &headerLine,
                             httpRequest &request) {
  size_t colonPos = headerLine.find(":");
  if (colonPos != std::string::npos) {
    std::string headerName = headerLine.substr(0, colonPos);
    std::string headerValue = headerLine.substr(colonPos + 1);

    request.headers[headerName] = headerValue;
  } else { // temporary for bad formatted & blank line
    return true;
  }

  return false;
}