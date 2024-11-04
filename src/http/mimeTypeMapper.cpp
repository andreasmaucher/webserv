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

void MimeTypeMapper::getFileExtension(HttpRequest &request) {
    
    request.file_name = getFileName(request);
    
    if (!request.file_name.empty()) {

        size_t pos = request.uri.find_last_of('.');
        if (pos != std::string::npos) {
            request.file_extension = request.uri.substr(pos + 1);
            request.is_cgi = isCGIRequest(request.file_extension);
            // it might have an extra directory between the valid route and the file name. How to handle?
            return;
        }

    }
    request.is_directory = true;
}

void MimeTypeMapper::getFileName(HttpRequest &request) {
// Extract the remaining path or file name from the URI

  if (request.uri.length() > request.route->uri.length()) {
    request.file_name = request.uri.substr(request.route->uri.length()); // might also be name of a folder instead of file_name or both!?!! needs to be checked in each method handling function
    
  }
//   } else if (request.method == "GET") {
//     request.file_name = DEFAULT_FILE; // alternatively respond with a default file when asking for a directory
//   }
  else {
    request.file_name = ""; // No additional file specified. Only valid for POST requests? or allow DELETING a directory?
  }
}

std::string MimeTypeMapper::getContentType(const std::string &extension) const {
    std::map<std::string, std::string>::const_iterator it = mime_types.find(extension);
    if (it != mime_types.end()) {
        return it->second;
    }
    return "";
}

bool MimeTypeMapper::isCGIRequest(const std::string &extension) {
   
    //std::string extension = getFileExtension(request);
    
    return cgi_extensions.find(extension) != cgi_extensions.end();
}

//check if the content type is allowed in that route
//check if the file extension is allowed in that route
//check if content type & file extension match
bool MimeTypeMapper::isContentTypeAllowed(HttpRequest &request, HttpResponse &response) {

    request.file_extension = getFileExtension(request);
    request.content_type = getContentType(file_extension);
    
    // there is a content type header && it matches the route's allowed content types
    if (!request.headers["Content-Type"].empty() && request.route->content_type.find(request.headers["Content-Type"]) != request.route->content_type.end()) {

        //there is no file name in the request uri OR there is a file name (with its extension) and it matches the request header
        //no file name case to be handled in each specific method handler!
       if (request.content_type.empty() || (!request.content_type.empty() && request.headers["Content-Type"] == request.content_type)) {
            return true;
        }
    }
    // no header present but file name/extension present and matching the route's allowed content type
    else if (request.headers["Content-Type"].empty() && (!request.content_type.empty() && request.route->content_type.find(request.content_type) != request.route->content_type.end())) {
    
        return true;
    }

    // specific method mandatory headers requirement checked previously in parser
    response.status_code = 415; // Unsupported Media Type
    return false;
}