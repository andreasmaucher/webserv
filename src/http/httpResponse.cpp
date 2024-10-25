#include "httpResponse.hpp"

HttpResponse::HttpResponse() : version(""), status_code(0), reason_phrase(""), body("") {}

// generates final response (formatted as one string)
std::string HttpResponse::generateResponseStr() {
    
    std::stringstream raw_response;
    raw_response << this->version << " " << this->status_code << " " << this->reason_phrase << "\r\n";
    if (this->status_code == 200)
        raw_response << "Content-Type: " << this->headers["Content-Type"] << "\r\n";
        raw_response << "Content-Length: " << this->headers["Content-Length"] << "\r\n";
        raw_response << "\r\n";
        raw_response << this->body;
    return raw_response.str();
}