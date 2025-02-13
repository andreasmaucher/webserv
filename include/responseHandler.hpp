#ifndef RESPONSEHANDLER_HPP
#define RESPONSEHANDLER_HPP

#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <map>
#include <ctime>
#include "httpRequest.hpp"
#include "httpResponse.hpp"
#include "server.hpp"
#include "mimeTypeMapper.hpp"
#include "cgi.hpp"
#include <dirent.h>
#include <sys/types.h>

// Processes the HTTP request and generates the HTTP response. Includes logic for:
// - Routing the request
// - Populating the HTTPResponse object based on the request
// *(Sending the response back to the client is done in a separate function called from the recv() loop)
class ResponseHandler
{
public:
    void processRequest(int &fd, Server &config, HttpRequest &request, HttpResponse &response);
    static void responseBuilder(HttpResponse &response);

private:
    // Routing
    static void routeRequest(int &fd, Server &server, HttpRequest &request, HttpResponse &response);
    static bool findMatchingRoute(Server &server, HttpRequest &request, HttpResponse &response);
    static bool isMethodAllowed(const HttpRequest &request, HttpResponse &response);
    static std::string createAllowedMethodsStr(const std::set<std::string> &methods);

    // Static Content Processing
    // GET request handlers
    static void staticContentHandler(HttpRequest &request, HttpResponse &response);
    static void serveStaticFile(HttpRequest &request, HttpResponse &response);
    // GET request helpers
    static void setFullPath(HttpRequest &request);
    static bool hasReadPermission(const std::string &file_path, HttpResponse &response);
    static bool readFile(HttpRequest &request, HttpResponse &response);
    // POST request handlers
    static void processFileUpload(HttpRequest &request, HttpResponse &response);
    static void writeToFile(HttpRequest &request, HttpResponse &response);
    // DELETE request handlers
    static void processFileDeletion(HttpRequest &request, HttpResponse &response);
    static void removeFile(HttpRequest &request, HttpResponse &response);

    // Response Building
    // static void responseBuilder(HttpResponse &response);
    static std::string getStatusMessage(int code);
    static void serveErrorPage(HttpResponse &response);

    // Shared Helper Functions
    static bool fileExists(HttpRequest &request, HttpResponse &response);
    static void constructFullPath(HttpRequest &request, HttpResponse &response);
    static bool handleSubdirectory(HttpRequest &request, HttpResponse &response, size_t &last_slash_pos);
    static void finalizeFullPath(HttpRequest &request, size_t &last_slash_pos);
    static void extractOrGenerateFilename(HttpRequest &request);
    static std::string generateTimestampName();
    static std::string sanitizeFileName(std::string &file_name);
    // Builder helpers
    static std::string generateDateHeader();
    static std::string buildFullPath(int status_code);
    static std::string read_error_file(std::string &file_path);
    // static void createHtmlBody(HttpResponse &response);
    static void generateDirectoryListing(const HttpRequest &request, HttpResponse &response);
};

#endif