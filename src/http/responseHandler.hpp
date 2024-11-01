#ifndef RESPONSEHANDLER_HPP
#define RESPONSEHANDLER_HPP

#define ROOT_DIR "www"
#define DEFAULT_FILE "index.html"

#include <iostream>
#include <string>
#include "httpRequest.hpp"
#include "httpResponse.hpp"
#include "../server/serverConfig.hpp"
#include "mimeTypeMapper.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <map>


// Processes the HTTP request and generates the HTTP response. Includes logic for:
// - Routing the request
// - Populating the HTTPResponse object based on the request
// *(Sending the response back to the client is done in a separate function called from the recv() loop)
class ResponseHandler {
    public:
        static void processRequest(ServerConfig &config, HttpRequest &request, HttpResponse &response);
        static void routeRequest(ServerConfig &config, HttpRequest &request, HttpResponse &response);
        static bool findMatchingRoute(const ServerConfig &config, HttpRequest &request, HttpResponse &response);
        static bool isMethodAllowed(const HttpRequest &request, HttpResponse &response);
        static std::string getFileName(HttpRequest &request);
        static void setPathToContent(HttpRequest &request, std::string &file_name);
        //static bool isCGIRequest(const std::string& path);
        static void staticContentHandler(HttpRequest &request, HttpResponse &response);
        static void serveStaticFile(HttpRequest &request, HttpResponse &response);
        static void handleFileUpload(const HttpRequest& request, HttpResponse& response);

        static bool hasReadPermission(const std::string &file_path, HttpResponse &response);
        static bool fileExists(const std::string &path, HttpResponse &response);
        static bool readFile(HttpRequest &request, HttpResponse &response);
        static void populateResponse(HttpRequest &request, HttpResponse &response);

        static void handleGet(HttpRequest &request, HttpResponse &response);
        
        static std::string createAllowedMethodsStr(const std::set<std::string> &methods);
        static std::string getStatusMessage(int code);
        static void createHtmlBody(HttpResponse &response);

};

#endif