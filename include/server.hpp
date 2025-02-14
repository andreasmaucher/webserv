#ifndef SERVER_HPP
#define SERVER_HPP

#define MAX_BODY_SIZE 1000000000
#define ROOT_DIR "www"
#define DEFAULT_FILE "index.html"
#define ERROR_PATH "/errors/"
#define POLL_TIMEOUT 1000

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>

class HttpRequest;

// Represents a single route. One for each of the location blocks in the config file
struct Route
{
    std::string uri;                    // The URI pattern, e.g., "/static"
    std::string path;                   // Physical directory path, e.g., "/var/www/static"
    std::set<std::string> methods;      // Allowed HTTP methods, e.g., {"GET", "HEAD"}
    std::set<std::string> content_type; // MIME type, e.g., "text/html"
    std::string redirect_uri;
    std::string index_file;
    bool directory_listing_enabled;
    bool is_cgi;
    bool autoindex;

    Route() : directory_listing_enabled(false), is_cgi(false), autoindex(false) {}
};

// Represents the overall server configuration
class Server
{
public:
    Server();
    Server(int listener_fd, std::string port, std::string name, std::string root_directory);

    // Getters
    const std::string &getPort() const;
    const std::string &getName() const;
    const int &getListenerFd() const;
    const std::string &getRootDirectory() const;
    const std::map<std::string, Route> &getRoutes() const;
    Route *getRoute(const std::string &uri); // Getter for a specific Route by URI
    const std::map<int, std::string> &getErrorPages() const;
    HttpRequest &getRequestObject(int &fd);
    // Setters
    void setRootDirectory(const std::string &root_directory);
    void setRoutes(const std::map<std::string, Route> &routes);
    void setRoute(const std::string &uri, const Route &route);
    void setErrorPages(const std::map<int, std::string> &error_pages);
    void setErrorPage(const int &code, const std::string &path);
    void setListenerFd(const int &listener_fd);
    void setRequestObject(int &fd, HttpRequest &request);
    void deleteRequestObject(const int &fd);
    void resetRequestObject(int &fd);
    void debugServer() const;
    void debugPrintRoutes() const;
    void clear();

    // changed to public temporarily to integrate between two branches
    int listener_fd; // Listening socket fd
    std::string port;
    std::string name;
    std::string host;
    std::string root_directory;
    std::string client_max_body_size;
    std::map<std::string, Route> routes;    // Mapping of URIs to Route objects
    std::map<int, std::string> error_pages; // Error pages mapped by status code
    std::string index;

private:
    // int listener_fd; //Listening socket fd
    // std::string port;
    // std::string name;
    // std::string host;
    // std::string root_directory;                      // Root directory for server files
    // std::vector<HttpRequest> httpRequests;
    std::map<int, HttpRequest> client_fd_to_request;
    // bool loadConfig(const std::string &config_file); // config file parser
};

// after parsing the config file, load all the error pages and other default htmls into the map of error_pages
// to avoid reading the file every time we need to send an error page (makes server faster)

#endif