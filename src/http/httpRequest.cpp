#include "httpRequest.hpp"

HttpRequest::HttpRequest() : raw_request(""), method(""), uri(""), path(""), version(""), headers(), body(""), route(NULL), file_name(""), file_extension(""), content_type(""), is_directory(false), is_cgi(false),  error_code(0), position(0), complete(false), headers_parsed(false), chunk_state()  {}

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
    //std::cout << "Headers Parsed: " << (this->headers_parsed ? "Yes" : "No") << "\n";
    std::cout << "Matched Route:\n  uri -> " << this->route->uri
              << "\n  path -> " << this->route->path
              << "\n  allowed methods -> " << this->route->methods
              << "\n  allowed content type -> " << this->route->content_type
              << "\n  redirection uri? -> " << this->route->redirect_uri
              << "\n  directory listing enabled? -> " << this->route->directory_listing_enabled
              << "\n  is cgi directory? -> " << (this->route->is_cgi ? "Yes" : "No")
              << std::endl;
    std::cout << "File Name: " << this->file_name << "\n";
    std::cout << "File Extension: " << this->file_extension << "\n";
    std::cout << "Content Type: " << this->content_type << "\n";
    std::cout << "Is CGI: " << (this->is_cgi ? "Yes" : "No") << "\n";
}

void HttpRequest::reset() {
  raw_request.clear();
  method.clear();
  uri.clear();
  path.clear();
  version.clear();
  headers.clear();
  body.clear();
  //route just holds an address to a Route object, actual routes are in ServerConfig class;
  file_name.clear();
  file_extension.clear();
  content_type.clear();
  is_cgi = false;
  error_code = 0;
  position = 0;
  complete = false;
  headers_parsed = false;
  chunk_state.reset(); 
}
