#include "testsHeader.hpp"

void test_routing()
{
    HttpResponse    response;
    HttpRequest     request = createFakeHttpRequest();
    ServerConfig    config = createFakeServerConfig();

    ResponseHandler::processRequest(config, request, response);
}

HttpRequest createFakeHttpRequest() {
    HttpRequest request;

    // Setting basic request-line values
    request.method = "POST";
    request.uri = "/static/upload.html";
    request.path = ""; // Server-side resolved path (to be filled by processRequest): /www/static/upload.html"
    request.version = "HTTP/1.1";

    // Adding common headers for a POST request
    request.headers["Host"] = "localhost";
    request.headers["User-Agent"] = "FakeBrowser/1.0";
    request.headers["Content-Type"] = "application/x-www-form-urlencoded";
    request.headers["Content-Length"] = "27";

    // Simulating raw request data (optional, but useful for testing)
    request.raw_request = 
        "POST /static/upload.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: FakeBrowser/1.0\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "name=test&value=hello%20world";

    // Body content for a POST request, e.g., form data
    request.body = "name=test&value=hello%20world";

    // Positioning attributes (used in chunked transfer, typically set to 0 initially)
    request.position = 0;
    request.error_code = 0;
    request.complete = true;
    request.headers_parsed = true;

    // Chunked transfer state (reset to default for this example)
    request.chunk_state.reset();

    return request;
}

ServerConfig createFakeServerConfig()
{
    ServerConfig config;

    // Root directory for the server
    config.root_directory = "/www";

    // Add a route for static files
    Route staticRoute;
    staticRoute.uri = "/static";
    staticRoute.path = config.root_directory + "/static";
    staticRoute.methods = {"GET", "HEAD"};
    staticRoute.content_type = {"text/html"};
    staticRoute.is_cgi = false;
    
    config.routes[staticRoute.uri] = staticRoute;

    // Add a route for images
    Route imageRoute;
    imageRoute.uri = "/images";
    imageRoute.path = config.root_directory + "/images";
    imageRoute.methods = {"GET"};
    imageRoute.content_type = {"image/png"};
    imageRoute.is_cgi = false;
    
    config.routes[imageRoute.uri] = imageRoute;

    // Add a route for a CGI script
    Route cgiRoute;
    cgiRoute.uri = "/cgi-bin";
    cgiRoute.path = config.root_directory + "/cgi-bin";
    cgiRoute.methods = {"GET", "POST"};
    cgiRoute.content_type = {"application/octet-stream"};
    cgiRoute.is_cgi = true;
    
    config.routes[cgiRoute.uri] = cgiRoute;

    // Define error pages
    config.error_pages[404] = config.root_directory + "/errors/404.html";
    config.error_pages[403] = config.root_directory + "/errors/403.html";
    config.error_pages[500] = config.root_directory + "/errors/500.html";

    return config;
}