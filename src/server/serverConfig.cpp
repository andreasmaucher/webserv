#include "../../include/serverConfig.hpp"

ServerConfig::ServerConfig() : port(0)
{
    // Initialize default values
    root_directory = ROOT_DIR;
}

ServerConfig::ServerConfig(const std::string &config_file) : port(0)
{
    if (!parseConfigFile(config_file))
    {
        std::cerr << "Error: Failed to parse config file" << std::endl;
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
    std::string line;
    std::string key, value;

    while (std::getline(config_file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        if (line.find("[[server]]") != std::string::npos)
        {
            config_file.seekg(-static_cast<int>(line.length()) - 1, std::ios::cur);
            return true;
        }

        if (line.find("[[server.location]]") != std::string::npos)
        {
            if (!parseLocationBlock(config_file))
                return false;
            continue;
        }

        std::cout << "Parsing line: " << line << std::endl;

        if (!parseKeyValue(line, key, value))
            continue;

        std::cout << "Key: '" << key << "' Value: '" << value << "'" << std::endl;

        if (key == "listen")
        {
            port = atoi(value.c_str());
            std::cout << "Set port to: " << port << std::endl;
        }
        else if (key == "host")
            host = value;
        else if (key == "root")
            setRootDirectory(value);
    }
    return true;
}

bool ServerConfig::parseConfigFile(const std::string &config_filename)
{
    std::ifstream config_file(config_filename.c_str());
    if (!config_file.is_open())
    {
        std::cerr << "Error: can't open config file" << std::endl;
        return false;
    }

    bool found_server = false;
    std::string line;

    while (std::getline(config_file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        if (line.find("[[server]]") != std::string::npos)
        {
            if (found_server)
            {
                // Create a new config and copy current values
                ServerConfig new_config;
                new_config.host = host;
                new_config.port = port;
                new_config.root_directory = root_directory;
                new_config.index = index;
                new_config.client_max_body_size = client_max_body_size;
                new_config.routes = routes;
                new_config.error_pages = error_pages;
                configs.push_back(new_config);

                // Reset current values for next server
                host.clear();
                port = 0;
                root_directory.clear();
                index.clear();
                client_max_body_size.clear();
                routes.clear();
                error_pages.clear();
            }

            if (!parseServerBlock(config_file))
                return false;

            found_server = true;
        }
    }

    if (found_server)
    {
        // Add the last server config
        ServerConfig new_config;
        new_config.host = host;
        new_config.port = port;
        new_config.root_directory = root_directory;
        new_config.index = index;
        new_config.client_max_body_size = client_max_body_size;
        new_config.routes = routes;
        new_config.error_pages = error_pages;
        configs.push_back(new_config);
    }

    std::cout << "Parsed " << configs.size() << " server configurations" << std::endl;

    // Use first config as main config
    if (!configs.empty())
    {
        host = configs[0].host;
        port = configs[0].port;
        root_directory = configs[0].root_directory;
        index = configs[0].index;
        client_max_body_size = configs[0].client_max_body_size;
        routes = configs[0].routes;
        error_pages = configs[0].error_pages;

        configs.erase(configs.begin());
    }

    std::cout << "First server port: " << port << std::endl;
    return found_server;
}

bool ServerConfig::parseLocationBlock(std::istream &config_file)
{
    Route route;
    std::string line;
    std::string key, value;

    while (std::getline(config_file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        // End of location block or start of new block
        if (line.find("[[") != std::string::npos)
        {
            config_file.seekg(-static_cast<int>(line.length()) - 1, std::ios::cur);
            routes[route.uri] = route;
            return true;
        }

        parseKeyValue(line, key, value);
        if (key.empty() || value.empty())
            continue;

        if (key == "path")
        {
            route.uri = value;
            route.path = value;
        }
        else if (key == "index")
            route.index_file = value;
        // Add other location configurations
    }
    routes[route.uri] = route;
    return true;
}

bool ServerConfig::parseKeyValue(const std::string &line, std::string &key, std::string &value)
{
    std::string::size_type pos = line.find('=');
    if (pos == std::string::npos)
    {
        key = "";
        value = "";
        return false;
    }

    key = line.substr(0, pos);
    value = line.substr(pos + 1);

    // Trim whitespace and quotes
    while (!key.empty() && (key[0] == ' ' || key[0] == '\t'))
        key.erase(0, 1);
    while (!key.empty() && (key[key.length() - 1] == ' ' || key[key.length() - 1] == '\t'))
        key.erase(key.length() - 1, 1);

    while (!value.empty() && (value[0] == ' ' || value[0] == '\t' || value[0] == '"'))
        value.erase(0, 1);
    while (!value.empty() && (value[value.length() - 1] == ' ' ||
                              value[value.length() - 1] == '\t' || value[value.length() - 1] == '"'))
        value.erase(value.length() - 1, 1);

    return true; // Return true if parsing was successful
}