#include "../../include/httpRequest.hpp"
#include "../../include/debug.hpp"

HttpRequest::HttpRequest() : raw_request(""), method(""), uri(""), path(""), version(""), headers(), body(""), route(NULL), file_name(""), file_extension(""), content_type(""), is_directory(false), is_cgi(false), error_code(0), position(0), complete(false), headers_parsed(false), chunk_state(), client_closed_connection(false) {}

void HttpRequest::reset()
{
  raw_request.clear();
  method.clear();
  uri.clear();
  path.clear();
  version.clear();
  headers.clear();
  body.clear();
  route = NULL;
  // route just holds an address to a Route object, actual routes are in ServerConfig class;
  file_name.clear();
  file_extension.clear();
  content_type.clear();
  is_directory = false;
  is_cgi = false;
  error_code = 0;
  position = 0;
  complete = false;
  headers_parsed = false;
  chunk_state.reset();
}

void HttpRequest::printRequest()
{
  DEBUG_MSG("=== MULTIPART FORM DEBUG ===", "");
  DEBUG_MSG("Method", this->method);
  DEBUG_MSG("Content-Type", this->headers["Content-Type"]);
  DEBUG_MSG("Content-Length", this->headers["Content-Length"]);
  DEBUG_MSG("Body size", this->body.size());
  
  // multipart boundary check for upload functionality
  std::string contentType = this->headers["Content-Type"];
  if (contentType.find("multipart/form-data") != std::string::npos) {
    DEBUG_MSG("Multipart form detected", "true");
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos != std::string::npos) {
      DEBUG_MSG("Boundary", contentType.substr(boundaryPos + 9));
    }
  }

  DEBUG_MSG("URI", this->uri);
  DEBUG_MSG("Version", this->version);

  DEBUG_MSG("Headers count", this->headers.size());
  for (std::map<std::string, std::string>::iterator header_iter = this->headers.begin();
       header_iter != this->headers.end(); ++header_iter)
  {
    DEBUG_MSG(header_iter->first, header_iter->second);
  }

  DEBUG_MSG("Body", this->body);
  DEBUG_MSG("Error Code", this->error_code);

  if (this->route)
  {
    DEBUG_MSG("Route URI", this->route->uri);
    DEBUG_MSG("Route Path", this->route->path);

    std::string methods_str;
    for (std::set<std::string>::iterator it = this->route->methods.begin();
         it != this->route->methods.end(); ++it)
    {
      methods_str += *it + " ";
    }
    DEBUG_MSG("Allowed Methods", methods_str);

    std::string content_types_str;
    for (std::set<std::string>::iterator it = this->route->content_type.begin();
         it != this->route->content_type.end(); ++it)
    {
      content_types_str += *it + " ";
    }
    DEBUG_MSG("Allowed Content Types", content_types_str);

    DEBUG_MSG("Redirect URI", this->route->redirect_uri);
    DEBUG_MSG("Directory Listing Enabled", this->route->directory_listing_enabled);
    DEBUG_MSG("Is CGI Directory", this->route->is_cgi);
  }

  DEBUG_MSG("File Name", this->file_name);
  DEBUG_MSG("File Extension", this->file_extension);
  DEBUG_MSG("Content Type", this->content_type);
  DEBUG_MSG("Is CGI", this->is_cgi);
}