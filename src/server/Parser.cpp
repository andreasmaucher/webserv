/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/01/08 00:40:16 by mrizhakov        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Parser.hpp"
#include "../../include/server.hpp"

Parser::Parser()
{
    // Initialize any necessary members
}

Parser::~Parser()
{
    // Cleanup if needed
}

void Parser::setRootDirectory(const std::string &root_directory)
{
    this->root_directory = root_directory;
}

bool Parser::parseLocationBlock(std::istream &config_file)
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

bool Parser::parseKeyValue(const std::string &line, std::string &key, std::string &value)
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

bool Parser::parseServerBlock(std::istream &config_file)
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

std::vector<Server> Parser::parseConfig(const std::string &config_file)
{
    std::vector<Server> servers_vector;

    std::ifstream file(config_file.c_str());
    if (!file.is_open())
    {
        std::cerr << "Error: can't open config file" << std::endl;
        return servers_vector;
    }

    bool found_server = false;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        if (line.find("[[server]]") != std::string::npos)
        {
            if (found_server)
            {
                // Create a new config and copy current values
                Server new_config(0, port, name, root_directory);

                new_config.host = host;
                new_config.port = port;
                new_config.root_directory = root_directory;
                // new_config.index = index;
                new_config.client_max_body_size = client_max_body_size;
                new_config.routes = routes;
                new_config.error_pages = error_pages;
                servers_vector.push_back(new_config);

                // Reset current values for next server
                host.clear();
                port = "0";
                root_directory.clear();
                index.clear();
                client_max_body_size.clear();
                routes.clear();
                error_pages.clear();
            }

            if (!parseServerBlock(file))
                return servers_vector;

            found_server = true;
        }
    }

    if (found_server)
    {
        // Add the last server config
        Server new_config;
        new_config.host = host;
        new_config.port = port;
        new_config.root_directory = root_directory;
        new_config.index = index;
        new_config.client_max_body_size = client_max_body_size;
        new_config.routes = routes;
        new_config.error_pages = error_pages;
        servers_vector.push_back(new_config);
    }

    std::cout << "Parsed " << servers_vector.size() << " server configurations" << std::endl;

    // Use first config as main config
    if (!servers_vector.empty())
    {
        host = servers_vector[0].host;
        port = servers_vector[0].port;
        root_directory = servers_vector[0].root_directory;
        index = servers_vector[0].index;
        client_max_body_size = servers_vector[0].client_max_body_size;
        routes = servers_vector[0].routes;
        error_pages = servers_vector[0].error_pages;

        servers_vector.erase(servers_vector.begin());
    }

    std::cout << "First server port: " << port << std::endl;
    // return found_server;

    // Example of config of fake servers

    // Server server_one(0, "8080", "server_one", "www");

    // // Add a route for static files
    // Route staticRoute;
    // staticRoute.uri = "/static";
    // staticRoute.path = server_one.getRootDirectory() + staticRoute.uri;
    // staticRoute.index_file = "index.html";
    // staticRoute.methods.insert("GET");
    // staticRoute.methods.insert("POST");
    // staticRoute.methods.insert("DELETE");
    // staticRoute.content_type.insert("text/plain");
    // staticRoute.content_type.insert("text/html");
    // staticRoute.is_cgi = false;
    // server_one.setRoute(staticRoute.uri, staticRoute);

    // // Add a route for more restrictive folder in static
    // Route restrictedstaticRoute;
    // restrictedstaticRoute.uri = "/static/restrictedstatic";
    // restrictedstaticRoute.path = server_one.getRootDirectory() + restrictedstaticRoute.uri;
    // restrictedstaticRoute.index_file = "index.html";
    // restrictedstaticRoute.methods.insert("GET");
    // restrictedstaticRoute.content_type.insert("text/plain");
    // restrictedstaticRoute.content_type.insert("text/html");
    // restrictedstaticRoute.is_cgi = false;
    // server_one.setRoute(restrictedstaticRoute.uri, restrictedstaticRoute);

    // test_servers.push_back(server_one);

    // Server server_two(0, "8081", "server_two", "www");

    // // Add a route for images
    // Route imageRoute;
    // imageRoute.uri = "/images";
    // imageRoute.path = server_one.getRootDirectory() + imageRoute.uri;
    // imageRoute.methods.insert("GET");
    // imageRoute.methods.insert("POST");
    // // imageRoute.methods.insert("DELETE");
    // imageRoute.content_type.insert("image/jpeg");
    // imageRoute.content_type.insert("image/png");
    // imageRoute.is_cgi = false;
    // server_two.setRoute(imageRoute.uri, imageRoute);

    // // Add a route for uploads
    // Route uploadsRoute;
    // uploadsRoute.uri = "/uploads";
    // uploadsRoute.path = server_one.getRootDirectory() + uploadsRoute.uri;
    // uploadsRoute.methods.insert("GET");
    // uploadsRoute.methods.insert("POST");
    // uploadsRoute.methods.insert("DELETE");
    // uploadsRoute.content_type.insert("image/jpeg");
    // uploadsRoute.content_type.insert("image/png");
    // uploadsRoute.content_type.insert("text/plain");
    // uploadsRoute.content_type.insert("text/html");
    // uploadsRoute.is_cgi = false;
    // server_two.setRoute(uploadsRoute.uri, uploadsRoute);

    // test_servers.push_back(server_two);

    return servers_vector;
}

// bool Parser::parseConfigFile(const std::string &config_filename)
// {
//     (void)config_filename;
//     return true;
// }