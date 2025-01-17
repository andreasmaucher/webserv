/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/01/17 20:44:16 by mrizhakov        ###   ########.fr       */
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
    std::string line, key, value;
    std::set<std::string> value_array;

    while (std::getline(config_file, line))
    {
        std::cout << " stuck in bool Parser::parseLocationBlock(std::istream &config_file)" << std::endl;
        if (line.empty() || line[0] == '#')
            continue;

        if (line.find("[[") != std::string::npos)
        {
            config_file.seekg(-static_cast<int>(line.length()) - 1, std::ios::cur);
            routes[route.uri] = route;
            return true;
        }
        std::cout << " didnt passed checkKeyPair(line)" << std::endl;

        ParseKeyValueResult result = checkKeyPair(line);
        std::cout << " passed checkKeyPair(line)" << std::endl;

        if (result == INVALID)
            return false;

        if (result == KEY_ARRAY_PAIR)
        {
            value_array.clear();
            if (!parseKeyArray(line, key, value_array))
            {
                std::cout << " passed !parseKeyArray(line, key, value_array)" << std::endl;

                return false;
            }
            if (key == "allow_methods")
            {
                route.methods.insert(value_array.begin(), value_array.end());
            }
        }
        else if (result == KEY_VALUE_PAIR || result == KEY_VALUE_PAIR_WITH_QUOTES)
        {
            if (!parseKeyValue(line, key, value))
                return false;
            if (!key.empty() && !value.empty())
            {
                if (key == "path")
                {
                    route.uri = value;
                    route.path = value;
                }
                else if (key == "index")
                {
                    route.index_file = value;
                }
            }
        }
    }
    routes[route.uri] = route;
    return true;
}

int Parser::checkValidQuotes(const std::string &line)
{
    int i = 0;
    int quoteNum = 0;

    while (line[i])
    {
        if (line[i] == '"')
            quoteNum++;
        i++;
    }
    return quoteNum;
}

bool Parser::checkValidSquareBrackets(const std::string &line)
{
    int i = 0;
    int starting_bracket = 0;
    int ending_bracket = 0;

    while (line[i])
    {
        if (line[i] == '[')
            starting_bracket++;
        if (line[i] == '[' && starting_bracket == 1)
            ending_bracket++;
        i++;
    }
    if (starting_bracket == 1 && ending_bracket == 1)
        return true;
    return false;
}

ParseKeyValueResult Parser::checkKeyPair(const std::string &line)
{
    std::string value;
    std::string::size_type pos = line.find('=');
    if (pos == std::string::npos)
        return INVALID;
    value = line.substr(pos + 1);

    if (checkValidQuotes(value) % 2 != 0)
        return INVALID;

    if (checkValidSquareBrackets(line) == true && checkValidQuotes(value) % 2 == 0)
        return KEY_ARRAY_PAIR;

    if (checkValidQuotes(value) == 2)
        return KEY_VALUE_PAIR_WITH_QUOTES;

    if (checkValidQuotes(value) == 0)
        return KEY_VALUE_PAIR;
    std::cout << " leaving checkKeyPair(line)" << std::endl;

    return INVALID;
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
    if (checkValidQuotes(key) != 0)
        return false;
    value = line.substr(pos + 1);
    if (checkValidQuotes(value) % 2 != 0 && checkValidQuotes(value) != 0)
        return false;

    // Trim whitespace and quotes from key - leading whitespaces
    while (!key.empty() && (key[0] == ' ' || key[0] == '\t'))
        key.erase(0, 1);
    // Trim whitespace and quotes from key - trailimg whitespaces
    while (!key.empty() && (key[key.length() - 1] == ' ' || key[key.length() - 1] == '\t'))
        key.erase(key.length() - 1, 1);
    // Trim whitespace and quotes from value - leading whitespaces and quotes
    while (!value.empty() && (value[0] == ' ' || value[0] == '\t' || value[0] == '"'))
        value.erase(0, 1);
    // Trim whitespace and quotes from value - trailing whitespaces and quotes
    while (!value.empty() && (value[value.length() - 1] == ' ' ||
                              value[value.length() - 1] == '\t' || value[value.length() - 1] == '"'))
        value.erase(value.length() - 1, 1);

    return true; // Return true if parsing was successful
}

bool Parser::parseKeyArray(const std::string &line, std::string &key, std::set<std::string> &value_set)
{
    std::string value;
    std::cout << "Inside parseKeyArray" << std::endl;

    std::string::size_type pos = line.find('=');
    if (pos == std::string::npos)
    {
        key = "";
        return false;
    }

    key = line.substr(0, pos);
    if (checkValidQuotes(key) != 0)
        return false;
    value = line.substr(pos + 1);
    checkValidSquareBrackets(value);
    if (checkValidQuotes(value) % 2 != 0 && checkValidSquareBrackets(value) == false)
        return false;

    // Trim whitespace and quotes from key - leading whitespaces
    while (!key.empty() && (key[0] == ' ' || key[0] == '\t'))
        key.erase(0, 1);
    // Trim whitespace and quotes from key - trailing whitespaces
    while (!key.empty() && (key[key.length() - 1] == ' ' || key[key.length() - 1] == '\t'))
        key.erase(key.length() - 1, 1);

    // Trim whitespace and opening square bracket in value
    while (!value.empty() && (value[0] == ' ' || value[0] == '\t' || value[0] == '['))
        value.erase(0, 1);
    // Trim whitespace and quotes from value - trailing whitespaces and quotes
    while (!value.empty() && (value[value.length() - 1] == ' ' || value[value.length() - 1] == '\t' || value[value.length() - 1] == ']'))
        value.erase(value.length() - 1, 1);

    size_t i = 0;
    size_t j = 0;
    bool inside_quotes = false;
    while (i < value.length() && value[i])
    {
        std::cout << "Current value: '" << value << "'" << std::endl;
        std::cout << "i: " << i << ", j: " << j << std::endl;
        // Skip non-quote characters
        while (i < value.length() && value[i] != '"' && !inside_quotes)
        {
            i++;
            j++;
        }

        if (i < value.length() && value[i] == '"')
        {
            inside_quotes = true;
            i++;
            j++;
        }

        // Find closing quote
        while (j < value.length() && value[j] != '"')
        {
            j++;
        }

        if (j < value.length()) // Found closing quote
        {
            inside_quotes = false;
            inside_quotes = false;
            std::string extracted = value.substr(i, j - i);
            std::cout << "Extracted: '" << extracted << "'" << std::endl;
            value_set.insert(extracted);
            i = j + 1;
            j = i;
        }
        else // No closing quote found
        {
            return false;
        }
    }
    return true; // Return true if parsing was successful
}
bool Parser::parseErrorBlock(std::istream &config_file, Server server)
{
    std::string line, key, value;
    char *end; // To capture the position where parsing stops
    int error_code_long;

    while (std::getline(config_file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        ParseKeyValueResult result = checkKeyPair(line);
        if (result == KEY_VALUE_PAIR)
        {
            if (!parseKeyValue(line, key, value))
                return false;
            if (!key.empty() && !value.empty())
            {
                error_code_long = strtol(key.c_str(), &end, 10); // Base 10 conversion
                if (error_code_long == 0)
                    return false;
                if (error_code_long > 0 && error_code_long < 1000)
                    server.error_pages.insert(std::make_pair((int)error_code_long, value));
                else
                    return false;
            }
        }
    }
    return true;
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
        {

            return false;
        }

        std::cout << "Key: '" << key << "' Value: '" << value << "'" << std::endl;

        if (key == "listen")
        {
            port = value;
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
                port.clear();
                root_directory.clear();
                index.clear();
                client_max_body_size.clear();
                routes.clear();
                error_pages.clear();
            }
            std::cout << "before if (!parseServerBlock(file) && !parseLocationBlock(file)) block" << std::endl;
            if (!parseServerBlock(file) && !parseLocationBlock(file))
                return servers_vector;
            std::cout << "after if (!parseServerBlock(file) && !parseLocationBlock(file)) block" << std::endl;

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
        // adding server only in case these 4 min variables are not empty
        if (!host.empty() && !port.empty() && !root_directory.empty() && !error_pages.empty())
            servers_vector.push_back(new_config);
    }

    std::cout << "First server port: " << port << std::endl;
    return servers_vector;
}
