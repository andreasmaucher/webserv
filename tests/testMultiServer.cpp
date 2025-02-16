#include "testsHeader.hpp"

std::vector <Server> createFakeServers()
{
    std::vector <Server> test_servers;

    Server server_one(0, "8080", "server_one", "www");

    // Add a route for static files
    Route staticRoute;
    staticRoute.uri = "/static";
    staticRoute.path = server_one.getRootDirectory() + staticRoute.uri;
    staticRoute.index_file = "index.html";
    staticRoute.methods.insert("GET");
    staticRoute.methods.insert("POST");
    staticRoute.methods.insert("DELETE");
    staticRoute.content_type.insert("text/plain");
    staticRoute.content_type.insert("text/html");
    staticRoute.is_cgi = false;
    server_one.setRoute(staticRoute.uri, staticRoute);

    // Add a route for more restrictive folder in static
    Route restrictedstaticRoute;
    restrictedstaticRoute.uri = "/static/restrictedstatic";
    restrictedstaticRoute.path = server_one.getRootDirectory() + restrictedstaticRoute.uri;
    restrictedstaticRoute.index_file = "index.html";
    restrictedstaticRoute.methods.insert("GET");
    restrictedstaticRoute.content_type.insert("text/plain");
    restrictedstaticRoute.content_type.insert("text/html");
    restrictedstaticRoute.is_cgi = false;
    server_one.setRoute(restrictedstaticRoute.uri, restrictedstaticRoute);

    test_servers.push_back(server_one);



    Server server_two(0, "8081", "server_two", "www");
    
    // Add a route for images
    Route imageRoute;
    imageRoute.uri = "/images";
    imageRoute.path = server_one.getRootDirectory() + imageRoute.uri;
    imageRoute.methods.insert("GET");
    imageRoute.methods.insert("POST");
    // imageRoute.methods.insert("DELETE");
    imageRoute.content_type.insert("image/jpeg");
    imageRoute.content_type.insert("image/png");
    imageRoute.is_cgi = false;
    server_two.setRoute(imageRoute.uri, imageRoute);

    // Add a route for uploads
    Route uploadsRoute;
    uploadsRoute.uri = "/uploads";
    uploadsRoute.path = server_one.getRootDirectory() + uploadsRoute.uri;
    uploadsRoute.methods.insert("GET");
    uploadsRoute.methods.insert("POST");
    uploadsRoute.methods.insert("DELETE");
    uploadsRoute.content_type.insert("image/jpeg");
    uploadsRoute.content_type.insert("image/png");
    uploadsRoute.content_type.insert("text/plain");
    uploadsRoute.content_type.insert("text/html");
    uploadsRoute.is_cgi = false;
    server_two.setRoute(uploadsRoute.uri, uploadsRoute);

    test_servers.push_back(server_two);

    return test_servers;
}
