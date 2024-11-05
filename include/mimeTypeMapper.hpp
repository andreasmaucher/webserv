#ifndef MIMETYPEMAPPER_HPP
#define MIMETYPEMAPPER_HPP

#include <string>
#include <map>
#include <set>
#include "httpRequest.hpp"
#include "httpResponse.hpp"

class MimeTypeMapper {
public:
    MimeTypeMapper();
    void extractFileExtension(HttpRequest &request);
    void extractFileName(HttpRequest &request);
    void findContentType(std::string &extension);
    bool isCGIRequest(const std::string &extension);
    bool isContentTypeAllowed(HttpRequest &request, HttpResponse &response);

private:
    std::map<std::string, std::string> mime_types;
    std::set<std::string> cgi_extensions;
    
    void initializeMimeTypes();
    void initializeCGIExtensions();
};

#endif