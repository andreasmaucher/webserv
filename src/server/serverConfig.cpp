#include "../../include/serverConfig.hpp"

ServerConfig::ServerConfig() : port(0)
{
    // Initialize default values
    root_directory = ROOT_DIR;
}

ServerConfig::ServerConfig(const std::string &config_file) : port(0)
{
    // Initialize default values
    root_directory = ROOT_DIR;

    // Parse the config file
    if (!parseConfigFile(config_file))
    {
        std::cerr << "Error: Failed to parse config file: " << config_file << std::endl;
    }
}

const std::string &ServerConfig::getRootDirectory() const
{
    return root_directory;
}

Route *ServerConfig::getRoute(const std::string &uri)
{
    std::map<std::string, Route>::iterator it = routes.find(uri);
    if (it != routes.end())
    {
        return &it->second; // Return pointer to the found Route
    }
    return NULL; // Return nullptr if not found
}

const std::map<std::string, Route> &ServerConfig::getRoutes() const
{
    return routes;
}

const std::map<int, std::string> &ServerConfig::getErrorPages() const
{
    return error_pages;
}

void ServerConfig::setRootDirectory(const std::string &root_directory)
{
    this->root_directory = root_directory;
}

void ServerConfig::setRoute(const std::string &uri, const Route &route)
{
    routes[uri] = route; // This will add a new route or update an existing one
}

// void ServerConfig::setRoutes(const std::map<std::string, Route> &routes) {
//     this->routes = routes;
// }

void ServerConfig::setErrorPage(const int &code, const std::string &path)
{
    error_pages[code] = path;
}

void ServerConfig::setErrorPages(const std::map<int, std::string> &error_pages)
{
    this->error_pages = error_pages;
}

bool ServerConfig::parseServerBlock(std::istream &config_file)
{
    (void)config_file;
    std::string line, key, value;
    std::cout << "Started parsing [[server]] block in config file" << std::endl;
    while (std::getline(config_file, line))
    {
        parseKeyValue(line, key, value);

        if (value.empty() || key.empty())
            continue;
        if (key == "listen")
        {
            port = std::atoi(value.c_str());
            std::cout << "Parsed port: " << port << std::endl;
        }
        else if (key == "host")
        {
            host = value;
            std::cout << "Parsed host: " << host << std::endl;
        }
        else if (key == "root")
        {
            root_directory = value;
            std::cout << "Parsed root_directory: " << root_directory << std::endl;
        }
        else if (key == "index")
        {
            index = value;
            std::cout << "Parsed index: " << index << std::endl;
        }
        else if (key == "client_max_body_size")
        {
            client_max_body_size = value;
            std::cout << "Parsed client_max_body_size: " << client_max_body_size << std::endl;
        }
        else if (line == "[[server.location]]")
        {
            std::cout << "Found [[server.location]] block in config file - parsing - " << line << std::endl;
            parseLocationBlock(config_file, routes[key]);
        }
    }
    //     name = "test"
    // listen = 8001
    // host = "127.0.0.1"
    // root = "www"
    // index = "index.html"
    // error_page = "www/errors/404.html"
    // client_max_body_size = 200000
    return true;
}

bool ServerConfig::parseConfigFile(const std::string &config_filename)
{
    std::ifstream config_file(config_filename);
    if (!config_file.is_open())
    {
        std::cerr << "Error: can't open config file" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(config_file, line))
    {
        std::cout << "Printing out each line read -- " << line << std::endl;

        // ignore commments and empty lines
        if (line.empty() || line[0] == '#')
        {
            std::cout << "Found empty line or comment in config file - skipping - " << line << std::endl;
            continue;
        }
        // parse server block if found one - [[server]]
        if (line.find("[[server]]") != std::string::npos)
        {
            std::cout << "Found [[server]] block in config file - parsing - " << line << std::endl;
            if (!parseServerBlock(config_file))
            {
                std::cerr << "Error parsing [[server]] block - " << line << std::endl;
                return false;
            }
        }
        if (line.find("[[server.location]]") != std::string::npos)
        {
            std::cout << "Found [[server]] block in config file - parsing - " << line << std::endl;
            if (!parseLocationBlock(config_file))
            {
                std::cerr << "Error parsing [[server]] block - " << line << std::endl;

                return false;
            }
        }
    }
    config_file.close();
    return false;
}

void trimWhitespace(std::string &str)
{
    while ((str[0] == ' ' || str[0] == '\t') && !str.empty())
        str.erase(0, 1);
    while ((str[str.length() - 1] == ' ' || str[str.length() - 1] == '\t') && !str.empty())
        str.erase(str.length() - 1, 1);
}

void trimWhitespaceQuotes(std::string &str)
{
    while ((str[0] == '"' || str[0] == ' ' || str[0] == '\t') && !str.empty())
        str.erase(0, 1);
    while ((str[str.length() - 1] == '"' || str[str.length() - 1] == ' ' || str[str.length() - 1] == '\t') && !str.empty())
        str.erase(str.length() - 1, 1);
}

unsigned int countQuotes(const std::string &str)
{
    unsigned int quote_count = 0;
    if (str.empty())
        return quote_count;

    size_t i = 0;
    while (i < str.length())
    {
        if (str[i] == '"')
            quote_count++;
        i++;
    }
    // if (quote_count != 2)
    //     return quote_count;
    return quote_count;
}

unsigned int countSquareBrackets(const std::string &str)
{
    unsigned int brackets_count = 0;
    if (str.empty())
        return brackets_count;

    size_t i = 0;
    while (i < str.length())
    {
        if (str[i] == '"')
            brackets_count++;
        i++;
    }
    // if (quote_count != 2)
    //     return quote_count;
    return brackets_count;
}

bool ServerConfig::parseKeyValue(const std::string &line, std::string &key, std::string &value)
{
    std::string::size_type position = line.find('=');
    key = "";
    value = "";

    // Check if line is empty, comment or no '=' found
    if (line.empty() || line[0] == '#' || position == std::string::npos)
        return false;
    // Get key and trim whitespace
    key = line.substr(0, position);
    trimWhitespace(key);
    // Get value and trim whitespace & remove quotes
    // If there are 2 quotes, trim whitespace & remove quotes
    // If there are no quotes, trim whitespace
    // Else signal that key-value pair is not valid, return false
    value = line.substr(position + 1);
    if (countQuotes(value) == 2)
        trimWhitespaceQuotes(value);
    else if (countQuotes(value) == 0)
        trimWhitespace(value);
    else
        return false;
    return true;
}

bool ServerConfig::parseLocationBlock(std::istream &config_file)
{
    (void)config_file;
    (void)route;
    return true;
}

// bool parseKeyArray(const std::string &line, std::string &key, std::set<std::string> &value)
// {
//     std::string value_str;
//     std::string::size_type position = line.find('=');
//     key = "";

//     // Check if line is empty, comment or no '=' found
//     if (line.empty() || line[0] == '#' || position == std::string::npos)
//         return false;

//     // Get key and trim whitespace
//     key = line.substr(0, position);
//     trimWhitespace(key);
//     value_str = line.substr(position + 1, line.length() - position - 1);
//     if (countSquareBrackets(value_str) != 0 || countSquareBrackets(value_str) != 2)
//         return false;
//     if (countSquareBrackets(value_str) == 0)
//     {
//         trimWhitespaceQuotes(value_str);
//         value.insert(value_str);
//     }
//     else if (countSquareBrackets(value_str) == 2)
//     {
//         trimWhitespaceQuotes(value_str);
//         value.insert(value_str);
//     }

//     while (position < value_str.length())
//     {
//     }
//     // Get value and trim whitespace & remove quotes
//     // If there are 2 quotes, trim whitespace & remove quotes
//     // If there are no quotes, trim whitespace
//     // Else signal that key-value pair is not valid, return false
//     value = line.substr(position + 1);
//     if (countSquareBrackets(value) != 2)
//         return false;
//     if (countQuotes(value) == 2)
//         trimWhitespaceQuotes(value);
//     else if (countQuotes(value) == 0)
//         trimWhitespace(value);
//     else
//         return false;
//     return true;
// }

// bool ServerConfig::loadConfig(const std::string &config_file) {
//     std::ifstream file(config_file);
//     if (!file.is_open()) {
//         std::cerr << "Error: could not open config file " << config_file << std::endl;
//         return false;
//     }

//     std::string line;
//     std::string uri;
//     std::string path;
//     std::string index_file;
//     std::set<std::string> methods;
//     std::set<std::string> content_type;
//     std::string redirect_uri;
//     bool directory_listing_enabled;
//     bool is_cgi;
//     while (std::getline(file, line)) {
//         if (line.empty() || line[0] == '#') {
//             continue;
//         }

//         std::istringstream iss(line);
//         std::string key;
//         iss >> key;
//         if (key == "root") {
//             iss >> root_directory;
//         } else if (key == "route") {
//             uri.clear();
//             path.clear();
//             methods.clear();
//             index_file.clear();
//             content_type.clear();
//             redirect_uri.clear();
//             directory_listing_enabled = false;
//             is_cgi = false;
//             std::string parameter;
//             while (iss >> parameter) {
//                 if (parameter == "uri") {
//                     iss >> uri;
//                 } else if (parameter == "path") {
//                     iss >> path;
//                 } else if (parameter == "method") {
//                     while (iss >> parameter) {
//                         methods.insert(parameter);
//                     }
//                 } else if (parameter == "content_type") {
//                     while (iss >> parameter) {
//                         content_type.insert(parameter);
//                     }
//                 } else if (parameter == "redirection") {
//                     iss >> redirect_uri;
//                 } else if (parameter == "index") {
//                         iss >> index_file;
//                 } else if (parameter == "directory_listing_enabled") {
//                     directory_listing_enabled = true;
//                 } else if (parameter == "is_cgi") {
//                     is_cgi = true;
//                 }
//             }
//             Route route;
//             route.uri = uri;
//             route.path = path;
//             route.methods = methods;
//             route.index = index_file;
//             route.content_type = content_type;
//             route.redirect_uri = redirect_uri;
//             route.directory_listing_enabled = directory_listing_enabled;
//             route.is_cgi = is_cgi;
//             routes[uri] = route;
//         }
//     }

//     file.close();
//     return true;
// }