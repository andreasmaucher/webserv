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
    staticRoute.methods.insert("GET");
    staticRoute.methods.insert("POST");
    staticRoute.methods.insert("DELETE");
    staticRoute.content_type.insert("text/plain");
    staticRoute.content_type.insert("text/html");
    staticRoute.is_cgi = false;
    
    config.setRoute(staticRoute.uri, staticRoute);

    // Add a route for images
    Route imageRoute;
    imageRoute.uri = "/images";
    imageRoute.path = config.getRootDirectory() + "/images";
    imageRoute.methods.insert("GET");
    imageRoute.content_type.insert("image/jpeg");
    imageRoute.is_cgi = false;
    
    config.setRoute(imageRoute.uri, imageRoute);

    // Add a route for a CGI script
    Route cgiRoute;
    cgiRoute.uri = "/cgi-bin";
    cgiRoute.path = config.getRootDirectory() + "/cgi-bin";
    // cgiRoute.methods = {"GET", "POST"};
    // cgiRoute.content_type = {"application/octet-stream"};
    cgiRoute.methods.insert("GET");
    cgiRoute.methods.insert("POST");
    cgiRoute.content_type.insert("application/octet-stream");
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

    run_test_("GET Index_File_Success", 200, createFakeHttpRequest("GET", "/static", "", "", ""), config);
    run_test_("GET Simple_Success", 200, createFakeHttpRequest("GET", "/static/upload.txt", "text/plain", "", ""), config);
    run_test_("GET No_Content_Type_Header_Success", 200, createFakeHttpRequest("GET", "/static/upload.txt", "", "", ""), config);
    run_test_("POST Simple_Success", 201, createFakeHttpRequest("POST", "/static/file.txt", "text/plain", "27", "name=test&value=hello%20world"), config);
    run_test_("POST No_File_Name_Success", 201, createFakeHttpRequest("POST", "/static", "text/plain", "27", "name=test&value=testttttest"), config);
    run_test_("DELETE Simple_Success", 200,createFakeHttpRequest("DELETE", "/static/file.txt", "text/plain", "", ""), config);

    run_test_("DELETE No_File_Specified", 400, createFakeHttpRequest("DELETE", "/static", "text/plain", "", ""), config); 
    run_test_("DELETE File_Not_Found", 404, createFakeHttpRequest("DELETE", "/static/missing.txt", "text/plain", "", ""), config);
    run_test_("GET File_Not_Found", 404, createFakeHttpRequest("GET", "/static/missing.txt", "text/plain", "", ""), config);
    run_test_("GET File_Forbidden", 403, createFakeHttpRequest("GET", "/images/secret.png", "image/png", "", ""), config);
    run_test_("GET Errors_File_Forbidden", 403, createFakeHttpRequest("GET", "/errors/400.html", "text/html", "", ""), config);
    run_test_("GET No_Matching_Route", 501, createFakeHttpRequest("GET", "/missing", "text/plain", "", ""), config);
    run_test_("GET Method_Not_Allowed_In_Route", 405, createFakeHttpRequest("DELETE", "/images/oli.jpg", "image/jpeg", "", ""), config);
    run_test_("POST File_Already_Exists", 409, createFakeHttpRequest("POST", "/static/upload.txt", "text/plain", "27", "name=test&value=hello%20world"), config);
    run_test_("POST No_File_Name_No_Body_No_Content_Type", 400, createFakeHttpRequest("POST", "/static", "", "", ""), config);
    run_test_("POST No_Content_Type_Header", 415, createFakeHttpRequest("POST", "/static/upload.txt", "", "27", "name=test&value=hello%20world"), config);
    run_test_("POST Content_Type_Header_Not_Allowed_In_Route", 415, createFakeHttpRequest("POST", "/static/upload.png", "image/png", "", "name=test&value=imageblabla"), config);
    run_test_("POST Content_Type_Not_Matching_File_Extension", 415, createFakeHttpRequest("POST", "/static/upload.txt", "text/html", "27", "name=test&value=hello%20world"), config);
    run_test_("POST No_Content_Type_Header_And_File_Extension_Not_Allowed_In_Route", 415, createFakeHttpRequest("POST", "/static/upload.py", "", "27", "name=test&value=hello%20world"), config);
    //test_serverError(config);

}


// Server error (500 error): Trigger an internal error scenario, e.g., bad CGI script handling.
// void test_serverError(ServerConfig &config) {
//     //ServerConfig config = createFakeServerConfig();
//     HttpRequest request = createFakeHttpRequest("GET", "", "", "", "");
//     HttpResponse response;

//     ResponseHandler::processRequest(config, request, response);

//     std::cout << response.generateRawResponseStr() << std::endl;
// }

// HttpRequest createFakeHttpRequest() {
//     HttpRequest request;

//     // Setting basic request-line values
//     request.method = "GET";
//     request.uri = "/images/oli.jpeg";
//     request.path = ""; // Server-side resolved path (to be filled by processRequest): /www/static/upload.html"
//     request.version = "HTTP/1.1";

//     // Adding common headers for a POST request
//     request.headers["Host"] = "localhost";
//     request.headers["User-Agent"] = "FakeBrowser/1.0";
//     //request.headers["Content-Type"] = "text/plain";
//     //request.headers["Content-Length"] = "27";

//     // Simulating raw request data (optional, but useful for testing)
//     // request.raw_request = 
//     //     "POST /static/upload.html HTTP/1.1\r\n"
//     //     "Host: localhost\r\n"
//     //     "User-Agent: FakeBrowser/1.0\r\n"
//     //     "Content-Type: application/x-www-form-urlencoded\r\n"
//     //     "Content-Length: 27\r\n"
//     //     "\r\n"
//     //     "name=test&value=hello%20world";

//     // // Body content for a POST request, e.g., form data
//     // request.body = "name=test&value=hello%20world";

//     // Positioning attributes (used in chunked transfer, typically set to 0 initially)
//     request.position = 0;
//     request.error_code = 0;
//     request.complete = true;
//     request.headers_parsed = true;

//     // Chunked transfer state (reset to default for this example)
//     request.chunk_state.reset();

//     return request;
// }