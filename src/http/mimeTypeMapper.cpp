#include "mimeTypeMapper.hpp"

MimeTypeMapper::MimeTypeMapper() {
    initializeMimeTypes();
    initializeCGIExtensions();
}

void MimeTypeMapper::initializeMimeTypes() {
    mime_types["html"] = "text/html";
    mime_types["htm"] = "text/html";
    mime_types["css"] = "text/css";
    mime_types["js"] = "application/javascript";
    mime_types["json"] = "application/json";
    mime_types["jpg"] = "image/jpeg";
    mime_types["jpeg"] = "image/jpeg";
    mime_types["png"] = "image/png";
    mime_types["gif"] = "image/gif";
    mime_types["txt"] = "text/plain";
    mime_types["pl"] = "application/x-perl";
    mime_types["py"] = "application/x-python";
    mime_types["php"] = "application/x-php";
    mime_types["cgi"] = "application/x-cgi";
}

void MimeTypeMapper::initializeCGIExtensions() {
    cgi_extensions.insert("pl");  // Perl scripts
    cgi_extensions.insert("py");  // Python scripts
    cgi_extensions.insert("php"); // PHP scripts
    cgi_extensions.insert("cgi"); // General CGI scripts
}

std::string MimeTypeMapper::getFileExtension(HttpRequest &request) {
    
    std::string file_name = getFileName(request);
    
    if (!file_name.empty()) {

        size_t pos = request.uri.find_last_of('.');
        if (pos != std::string::npos) {
            std::string extension = request.uri.substr(pos + 1);
            return extension;
        }
    }
    else
        return "";
}

std::string MimeTypeMapper::getFileName(HttpRequest &request) {
// Extract the remaining path or file name from the URI
  std::string file_name;
  if (request.uri.length() > request.route->uri.length()) {
    file_name = request.uri.substr(request.route->uri.length());
  } else if (request.method == "GET") {
    file_name = DEFAULT_FILE; // alternatively respond with a default file when asking for a directory
  }
  else {
    file_name = ""; // No additional file specified. Only valid for POST requests? or allow DELETING a directory?
  }
  return file_name;
}

std::string MimeTypeMapper::getContentType(const std::string &extension) const {
    std::map<std::string, std::string>::const_iterator it = mime_types.find(extension);
    if (it != mime_types.end()) {
        return it->second;
    }
    return "";
}

bool MimeTypeMapper::isCGIRequest(HttpRequest &request) {
   
    std::string extension = getFileExtension(request);
    
    return cgi_extensions.find(extension) != cgi_extensions.end();
}

//check if the content type is allowed in that route
//check if the file extension is allowed in that route
//check if content type & file extension match
bool MimeTypeMapper::isContentTypeAllowed(HttpRequest &request, HttpResponse &response) {

    std::string file_extension = getFileExtension(request);
    std::string content_type = getContentType(file_extension);
    
    // header matches route content type
    if (!request.headers["Content-Type"].empty() && request.route->content_type.find(request.headers["Content-Type"]) != request.route->content_type.end()) {
        
        //header matches file extension
       if (!content_type.empty() && request.headers["Content-Type"] == content_type) {
            return true;
        }
    }
    // no header present but file extension matches route content type
    else if (request.headers["Content-Type"].empty() && (!content_type.empty() && request.route->content_type.find(content_type) != request.route->content_type.end())) {
    
        return true;
    }

    response.status_code = 415; // Unsupported Media Type
    return false;
}