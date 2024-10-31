#include <string>
#include <map>
#include <set>
#include <vector>

// Represents a single route. One for each of the location blocks in the config file
struct Route {
    std::string uri;                       // The URI pattern, e.g., "/static"
    std::string path;                      // Physical directory path, e.g., "/var/www/static"
    std::set<std::string> methods;         // Allowed HTTP methods, e.g., {"GET", "HEAD"}
    std::set<std::string> content_type;              // MIME type, e.g., "text/html"
    bool is_cgi;                           // Is it a CGI route?

    Route() : is_cgi(false) {}
};

// Represents the overall server configuration
class ServerConfig {
    public:
    std::string root_directory;                      // Root directory for server files
    std::map<std::string, Route> routes;             // Mapping of URIs to Route objects
    std::map<int, std::string> error_pages;          // Error pages mapped by status code
};

//after parsing the config file, load all the error pages and other default htmls into the map of error_pages
//to avoid reading the file every time we need to send an error page (makes server faster)