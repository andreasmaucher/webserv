#include "httpRequest.hpp"

HttpRequest::HttpRequest() : method(""), uri(""), version(""), body(""), raw_request(""), error_code(0), headers_parsed(false), chunk_state()  {}

void HttpRequest::printRequest() {
    std::cout << "Method: " << this->method << "\n";
    std::cout << "URI: " << this->uri << "\n";
    std::cout << "Version: " << this->version << "\n";
    std::cout << "Headers:\n";
    for (std::map<std::string, std::string>::iterator header_iter = this->headers.begin(); header_iter != this->headers.end(); ++header_iter) {
      std::cout << "    " << header_iter->first << " " << header_iter->second << std::endl;
    }
    std::cout << "Body: " << this->body << "\n";
    std::cout << "Error Code: " << this->error_code << "\n";
    std::cout << "Headers Parsed: " << (this->headers_parsed ? "Yes" : "No") << "\n";
}