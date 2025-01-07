/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/01/07 16:28:06 by mrizhakov        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Parser.hpp"



std::vector <Server>  Parser::parseConfig(const std::string &config_file)
{
    std::vector <Server> servers_vector;
    
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
                Server new_config(port, host, name, root_directory);

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



bool Parser::parseConfigFile(const std::string &config_filename)
{
}