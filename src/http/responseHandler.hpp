#ifndef RESPONSEHANDLER_HPP
#define RESPONSEHANDLER_HPP

#define ROOT_DIR "www"
#define DEFAULT_FILE "index.html"

#include <iostream>
#include <string>
// #include <cstring>
// #include <map>
// #include <sstream>
// #include <vector>
#include "httpRequest.hpp"
#include "httpResponse.hpp"
#include "../server/serverConfig.hpp"

// Processes the HTTP request and generates the HTTP response. Includes logic for:
// - Routing the request
// - Populating the HTTPResponse object based on the request
// *(Sending the response back to the client is done in a separate function called from the recv() loop)
class ResponseHandler {
    public:
        static void processRequest(ServerConfig &config, HttpRequest &request, HttpResponse &response);
        static void routeRequest(ServerConfig &config, HttpRequest &request, HttpResponse &response);
        static void populateResponse(HttpRequest &request, HttpResponse &response);

        static void handleGet(HttpRequest &request, HttpResponse &response);
        
        static std::string createAllowedMethodsStr(const std::set<std::string> &methods);
        static std::string getStatusMessage(int code);
        static void createHtmlBody(HttpResponse &response);

};

#endif