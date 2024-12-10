#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#define MAX_BODY_SIZE 1000000
#define ROOT_DIR "www"
#define DEFAULT_FILE "index.html"
#define ERROR_PATH "/errors/"

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
    std::string redirect_uri;     
    std::string index_file;
    bool directory_listing_enabled;
    bool is_cgi;        

    Route() : is_cgi(false) {}
};

// Represents the overall server configuration
class ServerConfig {
    public:
        // ServerConfig();
        // ServerConfig(const std::string &config_file);
        
        // Getters
        const std::string &getRootDirectory() const;
        const std::map<std::string, Route> &getRoutes() const;
        Route* getRoute(const std::string &uri); // Getter for a specific Route by URI
        const std::map<int, std::string> &getErrorPages() const;
        // Setters
        void setRootDirectory(const std::string &root_directory);
        void setRoutes(const std::map<std::string, Route> &routes);
        void setRoute(const std::string &uri, const Route &route);
        void setErrorPages(const std::map<int, std::string> &error_pages);
        void setErrorPage(const int &code, const std::string &path);


    private:
        std::string server_name;
        int port_num;
        std::string root_directory;                      // Root directory for server files
        std::map<std::string, Route> routes;             // Mapping of URIs to Route objects
        std::map<int, std::string> error_pages;          // Error pages mapped by status code                        // Default index file
        
        bool loadConfig(const std::string &config_file); // config file parser
};

//after parsing the config file, load all the error pages and other default htmls into the map of error_pages
//to avoid reading the file every time we need to send an error page (makes server faster)

#endif