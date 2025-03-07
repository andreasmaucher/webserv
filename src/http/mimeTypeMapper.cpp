#include "../../include/mimeTypeMapper.hpp"
#include "../../include/debug.hpp"

MimeTypeMapper::MimeTypeMapper()
{
    initializeMimeTypes();
    initializeCGIExtensions();
}

void MimeTypeMapper::initializeMimeTypes()
{
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
    mime_types["ico"] = "image/x-icon";
}

void MimeTypeMapper::initializeCGIExtensions()
{
    cgi_extensions.insert("pl");  // Perl scripts
    cgi_extensions.insert("py");  // Python scripts
    cgi_extensions.insert("php"); // PHP scripts
    cgi_extensions.insert("cgi"); // General CGI scripts
}

void MimeTypeMapper::extractFileExtension(HttpRequest &request)
{
    extractFileName(request);

    if (!request.file_name.empty())
    {
        size_t pos = request.uri.find_last_of('.');
        if (pos != std::string::npos)
        {
            request.file_extension = request.uri.substr(pos + 1);
            request.is_cgi = isCGIRequest(request.file_extension);
            DEBUG_MSG("Extracted file extension", request.file_extension);
            return;
        }
    }
    request.is_directory = true;
}

void MimeTypeMapper::extractFileName(HttpRequest &request)
{
    // Michael : fixed version, hope it doesnt break anything else
    if (request.uri.length() > request.route->uri.length())
    {
        size_t start_index = request.route->uri.length();
        if (request.uri[start_index] == '/') // Check if there is a `/`
        {
            start_index += 1; // Skip the `/` only if it exists
        }
        request.file_name = request.uri.substr(start_index);
    }
    else
    {
        request.file_name = "";
    }

    // Old version from Carina, seemed to have a bug- would extrac secret.png and ecret.png
    // if (request.uri.length() > request.route->uri.length())
    // {
    //     request.file_name = request.uri.substr(request.route->uri.length() + 1);
    // }
    // else
    // {
    //     request.file_name = "";
    // }
    DEBUG_MSG("MimeTypeMapper: Extracted file name", request.file_name);
}

void MimeTypeMapper::findContentType(HttpRequest &request)
{
    DEBUG_MSG("Finding content type for extension", request.file_extension);
    std::map<std::string, std::string>::const_iterator it = mime_types.find(request.file_extension);
    if (it != mime_types.end())
    {
        request.content_type = it->second;
        DEBUG_MSG("Content type found", it->second);
        return;
    }
    request.content_type = "";
}

bool MimeTypeMapper::isCGIRequest(const std::string &extension)
{

    // std::string extension = getFileExtension(request);

    return cgi_extensions.find(extension) != cgi_extensions.end();
}

// check if the content type is allowed in that route
// check if the file extension is allowed in that route
// check if content type & file extension match
// TO DO: make this function part of the response handler!!
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
// bool MimeTypeMapper::isContentTypeAllowed(HttpRequest &request, HttpResponse &response)
// {
//     bool is_valid = false;
//     extractFileExtension(request);
//     findContentType(request);

//     // Add this new condition at the start
//     //! DIRECTORY
//     if (request.is_directory)
//     {
//         if (request.is_directory && request.method == "GET" && request.route->autoindex)
//         {
//             // Always allow GET requests to directories with autoindex enabled
//             return true;
//         }
//         // delete and post are not allowed for autoindex directory requests
//         else if (request.method != "GET")
//         {
//             response.status_code = 405;  // Method Not Allowed
//             response.reason_phrase = "Method Not Allowed";
//             response.setHeader("Allow", "GET");
//             return false;
//         }
//         return true;  // Allow GET even if autoindex is off (might have index.html)
//     }
//     if (request.is_directory)
//     {
//         DEBUG_MSG("URI type", "directory");
//         if (!request.headers["Content-Type"].empty())
//         {
//             bool header_matches = request.route->content_type.find(request.headers["Content-Type"]) != request.route->content_type.end();
//             DEBUG_MSG("Header content type matches route", header_matches);
//             is_valid = header_matches;
//         }
//         else
//         {
//             DEBUG_MSG("Content type in header", "None (allowed)");
//             is_valid = true;
//         }
//     }
//     else if (!request.headers["Content-Type"].empty())
//     {
//         DEBUG_MSG("Checking content type", request.content_type);
//         DEBUG_MSG("Request header Content-Type", request.headers["Content-Type"]);

//         bool header_matches_route = request.route->content_type.find(request.headers["Content-Type"]) != request.route->content_type.end();
//         bool header_matches_file = request.headers["Content-Type"] == request.content_type;

//         DEBUG_MSG("Header matches route", header_matches_route);
//         DEBUG_MSG("Header matches file", header_matches_file);

//         is_valid = header_matches_route && header_matches_file;
//     }
//     else if (request.headers["Content-Type"].empty() &&
//              (!request.content_type.empty() &&
//               request.route->content_type.find(request.content_type) != request.route->content_type.end()))
//     {
//         DEBUG_MSG("Content type validation", "No header but file extension matches route (allowed)");
//         is_valid = true;
//     }
//     // MICHAEL : added this part for files, need to check if logic is correct
//     else if (!request.is_directory)
//     {
//         DEBUG_MSG("URI type", "is a file");
//         if (!request.headers["Content-Type"].empty())
//         {
//             bool header_matches = request.route->content_type.find(request.headers["Content-Type"]) != request.route->content_type.end();
//             DEBUG_MSG("Header content type matches route", header_matches);
//             is_valid = header_matches;
//         }
//         else
//         {
//             DEBUG_MSG("Content type in header", "None (allowed)");
//             is_valid = true;
//         }
//     }
//     // MICHAEL : end of addition

//     if (!is_valid)
//     {
//         response.status_code = 415;
//         std::cout << "is response.status_code 415 set in isContentTypeAllowed?: " << response.status_code << std::endl;
//         DEBUG_MSG("Content type status", "Not allowed (415)");
//     }
//     return is_valid;
// }