#include "../../include/httpResponse.hpp"
#include "../../include/debug.hpp"

HttpResponse::HttpResponse() : version(""), status_code(0), reason_phrase(""), headers(), body(""), file_content_type(""), close_connection(false), complete(false) {}

void HttpResponse::setHeader(const std::string &header_name, const std::string &header_value)
{
  this->headers[header_name] = header_value;
}

// generates final response (formatted as one string)
std::string HttpResponse::generateRawResponseStr()
{
  std::string raw_string;
  // generate response status line:
  std::ostringstream oss;
  oss << this->status_code;
  raw_string = this->version + " " + oss.str() + " " + this->reason_phrase + "\r\n";
  DEBUG_MSG_2("SG5 ", "");

  // add headers
  for (std::map<std::string, std::string>::const_iterator it = this->headers.begin();
       it != this->headers.end(); ++it)
  {
    // Use simple logging to avoid potential issues in DEBUG_MSG_2
    raw_string += it->first + ": " + it->second + "\r\n";
    DEBUG_MSG_2("SG10 ", "");

  }
  DEBUG_MSG_2("SG9 ", "");

  if (!headers.empty())
  {
    DEBUG_MSG_2("SG7 ", "");

    raw_string += "\r\n";
  }
  //! REDIRECTS
  // only add body for non-redirects
  bool is_redirect = (status_code >= 300 && status_code < 400);
  if (!this->body.empty() && !is_redirect)
  {
    DEBUG_MSG_2("SG8 ", "");
    raw_string += this->body; // + "/r/n" ??
  }
  return raw_string;
}