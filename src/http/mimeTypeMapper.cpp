#include "../../include/mimeTypeMapper.hpp"

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

void MimeTypeMapper::extractFileExtension(HttpRequest &request) {
    
    extractFileName(request);
    
    if (!request.file_name.empty()) {

        size_t pos = request.uri.find_last_of('.');
        if (pos != std::string::npos) {
            request.file_extension = request.uri.substr(pos + 1);
            request.is_cgi = isCGIRequest(request.file_extension);
            // it might have an extra directory between the valid route and the file name. How to handle?
            std::cout << "Extracted file extension: " << request.file_extension << std::endl;
            return;
        }
    }
    request.is_directory = true;
}

void MimeTypeMapper::extractFileName(HttpRequest &request) {
// Extract the remaining path or file name from the URI

  if (request.uri.length() > request.route->uri.length()) {
    request.file_name = request.uri.substr(request.route->uri.length() + 1); // might also be name of a folder instead of file_name or both!?!! needs to be checked in each method handling function
  }
//   } else if (request.method == "GET") {
//     request.file_name = DEFAULT_FILE; // alternatively respond with a default file when asking for a directory
//   }
  else {
    request.file_name = ""; // No additional file specified. Only valid for POST requests? or allow DELETING a directory?
  }

  std::cout << "Extracted file name: " << request.file_name << std::endl;
}

void MimeTypeMapper::findContentType(HttpRequest &request) {
    std::map<std::string, std::string>::const_iterator it = mime_types.find(request.file_extension);
    std::cout << "Finding content type for extension: " << request.file_extension << std::endl;
    if (it != mime_types.end()) {
        std::cout << "Content type found: " << it->second << std::endl;
        request.content_type = it->second;
        return;
    }
    request.content_type = "";
}

bool MimeTypeMapper::isCGIRequest(const std::string &extension) {
   
    //std::string extension = getFileExtension(request);
    
    return cgi_extensions.find(extension) != cgi_extensions.end();
}

//check if the content type is allowed in that route
//check if the file extension is allowed in that route
//check if content type & file extension match
//TO DO: make this function part of the response handler!!
bool MimeTypeMapper::isContentTypeAllowed(HttpRequest &request, HttpResponse &response) {

    bool is_valid = false;
    //instantiate mapper here and call the methods for extracting file extension and content type
    extractFileExtension(request);
    findContentType(request);
    
    if (request.is_directory) {
        std::cout << "Uri is a directory" << std::endl;
        if (!request.headers["Content-Type"].empty()) {
            if (request.route->content_type.find(request.headers["Content-Type"]) != request.route->content_type.end()) {
                std::cout << "Content type in header (" << request.headers["Content-Type"] << ") matches route's allowed content types" << std::endl;
                is_valid = true;
                //return true;
            }
            else {
                std::cout << "Content type in header (" << request.headers["Content-Type"] << ") doesn't match route's allowed content types" << std::endl;
                is_valid = false;
                //return false;
            }
        }
        //check if directory listing on or just return default file??
        else {
            std::cout << "No content type in header\nAllowed for now..." << std::endl;
            is_valid = true;
            //return true;
        }
    
    }
    
    
    // there is a content type header
    else if (!request.headers["Content-Type"].empty()) {
        std::cout << "Checking if content type '" << request.content_type << "' is allowed" << std::endl;
        std::cout << "Content Type Header found in request: '" << request.headers["Content-Type"] << "'" << std::endl;
        // it matches the route's allowed content types
        if (request.route->content_type.find(request.headers["Content-Type"]) != request.route->content_type.end() && request.headers["Content-Type"] == request.content_type) {
            std::cout << "Matches route's allowed content types" << std::endl;
            std::cout << "Matches file extension \nContent type allowed"<< std::endl;
            is_valid = true;
            //return true;
        }
        else {
            bool header_matches_route = request.route->content_type.find(request.headers["Content-Type"]) != request.route->content_type.end();
            bool header_matches_file = request.headers["Content-Type"] == request.content_type;
            std::cout << "Header matches route: " << header_matches_route << std::endl;
            std::cout << "Header matches file: " << header_matches_file << std::endl;
            std::cout << "Not allowed" << std::endl;
            is_valid = false;
            //return false;
        }
    }
    // no header present but file name/extension present and matching the route's allowed content type
    else if (request.headers["Content-Type"].empty() && (!request.content_type.empty() && request.route->content_type.find(request.content_type) != request.route->content_type.end())) {
        std::cout << "No content type in header but file extension matches route's allowed content types\nContent type allowed" << std::endl;
        is_valid = true;
        //return true;
    }

    if (!is_valid) {
        response.status_code = 415; // Unsupported Media Type
        std::cout << "->Content type not allowed" << std::endl;
        // specific method mandatory headers requirement checked previously in parser
        //return false;
    }
    // std::cout << "Content type not allowed" << std::endl;
    // // specific method mandatory headers requirement checked previously in parser
    // response.status_code = 415; // Unsupported Media Type
    //return false;
    return is_valid;
}