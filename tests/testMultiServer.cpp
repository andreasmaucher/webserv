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
    // Route nocontentallowedRoute;
    // nocontentallowedRoute.uri = "/nocontentallowed";
    // nocontentallowedRoute.path = server_one.getRootDirectory() + nocontentallowedRoute.uri;
    // nocontentallowedRoute.methods.insert("GET");
    // nocontentallowedRoute.methods.insert("POST");
    // nocontentallowedRoute.methods.insert("DELETE");
    // nocontentallowedRoute.is_cgi = false;
    
    // server_one.setRoute(nocontentallowedRoute.uri, nocontentallowedRoute);

    // Route nomethodallowedRoute;
    // nomethodallowedRoute.uri = "/nomethodallowed";
    // nomethodallowedRoute.path = server_one.getRootDirectory() + nomethodallowedRoute.uri;
    // nomethodallowedRoute.content_type.insert("text/plain");
    // nomethodallowedRoute.content_type.insert("text/html");
    // nomethodallowedRoute.is_cgi = false;
    
    // server_one.setRoute(nomethodallowedRoute.uri, nomethodallowedRoute);

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
    server_one.setRoute(imageRoute.uri, imageRoute);

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
    server_one.setRoute(uploadsRoute.uri, uploadsRoute);

    test_servers.push_back(server_two);

    // // Add a route for a CGI script
    // Route cgiRoute;
    // cgiRoute.uri = "/cgi-bin";
    // cgiRoute.path = server_one.getRootDirectory() + cgiRoute.uri;
    // cgiRoute.methods.insert("GET");
    // cgiRoute.methods.insert("POST");
    // cgiRoute.content_type.insert("application/octet-stream");
    // cgiRoute.is_cgi = true;
    // server_one.setRoute(cgiRoute.uri, cgiRoute);

    // Define error pages
    // server_one.setErrorPage(404, server_one.getRootDirectory() + "/errors/404.html");
    // server_one.setErrorPage(403, server_one.getRootDirectory() + "/errors/403.html");
    // server_one.setErrorPage(500, server_one.getRootDirectory() + "/errors/500.html");

    return test_servers;
}
