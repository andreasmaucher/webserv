#include "../../include/httpResponse.hpp"

HttpResponse::HttpResponse() : version(""), status_code(0), reason_phrase(""), headers(), body(""), file_content_type("") {}


void HttpResponse::setHeader(const std::string &header_name, const std::string &header_value) {
  this->headers[header_name] = header_value;
}

// generates final response (formatted as one string)
std::string HttpResponse::generateRawResponseStr() {
  std::string raw_string;
  //generate response status line:
  std::ostringstream oss;
  oss << this->status_code;
  raw_string = this->version + " " + oss.str() + " " + this->reason_phrase + "\r\n";
  //add headers
  for (std::map<std::string, std::string>::const_iterator it = this->headers.begin(); it != this->headers.end(); ++it) {
   raw_string += it->first + ": " + it->second + "\r\n";
  }
  if (!headers.empty()) {
   raw_string += "\r\n";
  }
  if (!this->body.empty()) {
   raw_string += this->body; // + "/r/n" ??
  }

  return raw_string;
}