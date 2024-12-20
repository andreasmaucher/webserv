#include "testsHeader.hpp"

ServerConfig createFakeServerConfig()
{
    ServerConfig config;

    // Root directory for the server
    config.setRootDirectory("www");

    // Add a route for static files
    Route staticRoute;
    staticRoute.uri = "/static";
    staticRoute.path = config.getRootDirectory() + staticRoute.uri;
    staticRoute.index_file = "index.html";
    staticRoute.methods.insert("GET");
    staticRoute.methods.insert("POST");
    staticRoute.methods.insert("DELETE");
    staticRoute.content_type.insert("text/plain");
    staticRoute.content_type.insert("text/html");
    staticRoute.is_cgi = false;
    
    config.setRoute(staticRoute.uri, staticRoute);

    // Add a route for more restrictive folder in static
    Route restrictedstaticRoute;
    restrictedstaticRoute.uri = "/static/restrictedstatic";
    restrictedstaticRoute.path = config.getRootDirectory() + restrictedstaticRoute.uri;
    restrictedstaticRoute.index_file = "index.html";
    restrictedstaticRoute.methods.insert("GET");
    restrictedstaticRoute.content_type.insert("text/plain");
    restrictedstaticRoute.content_type.insert("text/html");
    restrictedstaticRoute.is_cgi = false;
    
    config.setRoute(restrictedstaticRoute.uri, restrictedstaticRoute);


    Route nocontentallowedRoute;
    nocontentallowedRoute.uri = "/nocontentallowed";
    nocontentallowedRoute.path = config.getRootDirectory() + nocontentallowedRoute.uri;
    nocontentallowedRoute.methods.insert("GET");
    nocontentallowedRoute.methods.insert("POST");
    nocontentallowedRoute.methods.insert("DELETE");
    nocontentallowedRoute.is_cgi = false;
    
    config.setRoute(nocontentallowedRoute.uri, nocontentallowedRoute);

    Route nomethodallowedRoute;
    nomethodallowedRoute.uri = "/nomethodallowed";
    nomethodallowedRoute.path = config.getRootDirectory() + nomethodallowedRoute.uri;
    nomethodallowedRoute.content_type.insert("text/plain");
    nomethodallowedRoute.content_type.insert("text/html");
    nomethodallowedRoute.is_cgi = false;
    
    config.setRoute(nomethodallowedRoute.uri, nomethodallowedRoute);

    // Add a route for images
    Route imageRoute;
    imageRoute.uri = "/images";
    imageRoute.path = config.getRootDirectory() + imageRoute.uri;
    imageRoute.methods.insert("GET");
    imageRoute.methods.insert("POST");
    imageRoute.methods.insert("DELETE");
    imageRoute.content_type.insert("image/jpeg");
    imageRoute.content_type.insert("image/png");
    imageRoute.is_cgi = false;
    
    config.setRoute(imageRoute.uri, imageRoute);

    // Add a route for uploads
    Route uploadsRoute;
    uploadsRoute.uri = "/uploads";
    uploadsRoute.path = config.getRootDirectory() + uploadsRoute.uri;
    uploadsRoute.methods.insert("GET");
    uploadsRoute.methods.insert("POST");
    uploadsRoute.methods.insert("DELETE");
    uploadsRoute.content_type.insert("image/jpeg");
    uploadsRoute.content_type.insert("image/png");
    uploadsRoute.content_type.insert("text/plain");
    uploadsRoute.content_type.insert("text/html");
    uploadsRoute.is_cgi = false;
    config.setRoute(uploadsRoute.uri, uploadsRoute);

    // Add a route for a CGI script
    Route cgiRoute;
    cgiRoute.uri = "/cgi-bin";
    cgiRoute.path = "/cgi-bin";
    cgiRoute.methods.insert("GET");
    cgiRoute.methods.insert("POST");
    cgiRoute.content_type.insert("application/octet-stream");
    cgiRoute.content_type.insert("application/x-python");
    cgiRoute.content_type.insert("text/plain"); //! ANDY
    //! what other content types do I need to add here?
    cgiRoute.is_cgi = true;
    config.setRoute(cgiRoute.uri, cgiRoute);

    // Define error pages
    // config.setErrorPage(404, config.getRootDirectory() + "/errors/404.html");
    // config.setErrorPage(403, config.getRootDirectory() + "/errors/403.html");
    // config.setErrorPage(500, config.getRootDirectory() + "/errors/500.html");

    return config;
}

HttpRequest createFakeHttpRequest(const std::string &method, const std::string &uri, const std::string &content_type, const std::string &content_length, const std::string &body) {
    
    HttpRequest request;
    
    request.method = method;
    request.uri = uri;
    request.version = "HTTP/1.1";
    request.headers["Host"] = "localhost";
    request.headers["User-Agent"] = "FakeBrowser/1.0";
    request.headers["Content-Type"] = content_type;
    request.headers["Content-Length"] = content_length;
    request.body = body;
    request.position = 0;
    request.error_code = 0;
    request.complete = true;
    request.headers_parsed = true;
    request.chunk_state.reset();

    return request;
}

void logTestResult(const std::string &testName, bool &passed, const std::string &result) {
    std::ofstream logFile("test_results.log", std::ios::app); // Open in append mode
    if (logFile.is_open()) {
        logFile << "TEST: " << testName << " - RESULT: " << (passed ? "Passed" : "Failed") << "\nExecution Output:\n" << result << "\n" << std::endl;
        logFile.close();
    } else {
        std::cerr << "Error opening log file!" << std::endl;
    }
}

void run_test_(const std::string &test_name, const int expected_status_code, HttpRequest request, const ServerConfig &config) {

    HttpResponse response;

    ResponseHandler::processRequest(config, request, response);
    
    bool passed = response.status_code == expected_status_code;
    logTestResult(test_name, passed, response.generateRawResponseStr());

}

void test_responseHandler()
{
    ServerConfig    config = createFakeServerConfig();

    run_test_("GET Index_File", 200, createFakeHttpRequest("GET", "/static", "", "", ""), config);
    run_test_("GET Simple", 200, createFakeHttpRequest("GET", "/static/example.txt", "text/plain", "", ""), config);
    run_test_("GET No_Content_Type_Header", 200, createFakeHttpRequest("GET", "/static/example.txt", "", "", ""), config);
    run_test_("POST Simple", 201, createFakeHttpRequest("POST", "/uploads/file.txt", "text/plain", "10", "a file"), config);
    run_test_("POST No_File_Name", 201, createFakeHttpRequest("POST", "/uploads", "text/plain", "31", "name=test&value=testttttest"), config);
    run_test_("POST No_Content_Type_Header", 201, createFakeHttpRequest("POST", "/uploads/no_content_type.txt", "", "27", "name=test&value=hello%20world"), config);
    run_test_("DELETE Simple", 200,createFakeHttpRequest("DELETE", "/uploads/file.txt", "text/plain", "", ""), config);

    run_test_("GET File_Not_Found", 404, createFakeHttpRequest("GET", "/static/missing.txt", "text/plain", "", ""), config);
    //make sure that file has no read permission for next test
    //run_test_("GET File_Forbidden", 403, createFakeHttpRequest("GET", "/images/secret.png", "image/png", "", ""), config);
    //run_test_("DELETE File_Forbidden", 403, createFakeHttpRequest("DELETE", "/static/asecret.txt", "text/plain", "", ""), config);

    run_test_("POST File_Already_Exists", 409, createFakeHttpRequest("POST", "/uploads/dont_delete.txt", "text/plain", "15", "change file"), config);
    run_test_("POST No_Body_No_Content_Type", 400, createFakeHttpRequest("POST", "/uploads", "", "", ""), config);
    
    run_test_("DELETE No_File_Specified", 501, createFakeHttpRequest("DELETE", "/uploads", "text/plain", "", ""), config); 
    run_test_("DELETE File_Not_Found", 404, createFakeHttpRequest("DELETE", "/uploads/missing.txt", "text/plain", "", ""), config);
    //DELETE File_Forbidden
    
    run_test_("No_Matching_Route", 404, createFakeHttpRequest("GET", "/missing", "text/plain", "", ""), config);
    run_test_("Method_Not_Allowed_In_Route", 405, createFakeHttpRequest("DELETE", "/images/oli.jpg", "", "", ""), config);
    run_test_("Content_Type_Header_Not_Allowed_In_Route", 415, createFakeHttpRequest("GET", "/static", "image/png", "", ""), config);
    run_test_("Content_Type_Not_Matching_File_Extension", 415, createFakeHttpRequest("POST", "/uploads/upload.txt", "text/html", "27", "name=test&value=hello%20world"), config);
    run_test_("POST No_Content_Type_Header_And_File_Extension_Not_Allowed_In_Route", 415, createFakeHttpRequest("POST", "/uploads/script.py", "", "27", "name=test&value=hello%20world"), config);
    
    //test_serverError(config);

}