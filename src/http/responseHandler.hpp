#ifndef RESPONSEHANDLER_HPP
#define RESPONSEHANDLER_HPP

#include <iostream>
#include <string>
// #include <cstring>
// #include <map>
// #include <sstream>
// #include <vector>
#include "httpRequest.hpp"
#include "httpResponse.hpp"

// Processes the HTTP request and generates the HTTP response. Includes logic for:
// - Routing the request
// - Populating the HTTPResponse object based on the request
// *(Sending the response back to the client is done in a separate function called from the recv() loop)
class ResponseHandler {
    public:
        static void processRequest(HttpRequest &request, HttpResponse &response);
        static void routeRequest(HttpRequest &request, HttpResponse &response);
        static void populateResponse(HttpRequest &request, HttpResponse &response);

        static std::string getStatusMessage(int code);

};

#endif