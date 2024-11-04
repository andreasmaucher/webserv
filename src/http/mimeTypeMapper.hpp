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
    std::string getContentType(const std::string &extension) const;
    void getFileName(HttpRequest &request);
    std::string getFileExtension(HttpRequest &request);
    bool isContentTypeAllowed(HttpRequest &request, HttpResponse &response);
    bool isCGIRequest(HttpRequest &request);

private:
    std::map<std::string, std::string> mime_types;
    std::set<std::string> cgi_extensions;
    void initializeMimeTypes();
    void initializeCGIExtensions();
};

#endif