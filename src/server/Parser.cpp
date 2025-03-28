/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: amaucher <amaucher@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Parser.hpp"
#include "../../include/server.hpp"
#include "../../include/debug.hpp"


Parser::Parser() {}

Parser::~Parser() {}

void Parser::setRootDirectory(const std::string &root_directory)
{
    this->root_directory = root_directory;
}

bool Parser::parseLocationBlock(std::istream &config_file, Server &server)
{
    Route route;
    std::string line, key, value;
    std::set<std::string> value_array;

    while (std::getline(config_file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        if (line.find("[[server]]") != std::string::npos || line.find("[[server.error_page]]") != std::string::npos)
        {
            if (!route.path.empty() && !route.methods.empty())
            {
                location_bloc_ok = true;
                server.setRoute(route.uri, route);
                config_file.seekg(-static_cast<int>(line.length()) - 1, std::ios::cur);
                return true;
            }
            else
            {
                location_bloc_ok = false;
                config_file.seekg(-static_cast<int>(line.length()) - 1, std::ios::cur);
                return false;
            }
        }
        if (line.find("[[server.location]]") != std::string::npos)
        {
            if (!route.path.empty() && !route.methods.empty())
            {
                server.setRoute(route.uri, route);
                route = Route();
                continue;
            }
            else
            {
                route = Route();
                continue;
            }
        }
        ParseKeyValueResult result = checkKeyPair(line);
        if (result == EMPTY_LINE || line[0] == '#')
            continue;
        else if (result == INVALID)
            return false;
        else if (result == KEY_ARRAY_PAIR)
        {
            value_array.clear();

            if (!parseKeyArray(line, key, value_array))
            {
                return false;
            }
            if (key == "allow_methods")
            {
                route.methods.insert(value_array.begin(), value_array.end());
            }
            if (key == "content_type")
            {
                route.content_type.insert(value_array.begin(), value_array.end());
            }
        }
        else if (result == KEY_VALUE_PAIR || result == KEY_VALUE_PAIR_WITH_QUOTES)
        {
            if (!parseKeyValue(line, key, value))
                return false;
            if (!key.empty() && !value.empty())
            {
                if (key == "uri")
                {
                    route.uri = value;
                    route.path = server.getRootDirectory() + route.uri;
                }
                else if (key == "redirect")
                {
                    route.redirect_uri = value;
                }
                else if (key == "index")
                {
                    route.index_file = value;
                }
                if (key == "is_cgi" && value == "true")
                {
                    route.is_cgi = true;
                }
                if (key == "directory_listing_enabled" && value == "true")
                {
                    route.directory_listing_enabled = true;
                }
                if (key == "autoindex")
                {
                    route.autoindex = (value == "true" || value == "on" || value == "1");
                }
            }
        }
    }

    if (!route.path.empty() && !route.methods.empty())
    {
        location_bloc_ok = true;
        server.setRoute(route.uri, route);
        return true;
    }
    return true;
}

bool Parser::checkMaxBodySize(const std::string &value)
{
    char *end;
    long value_long = strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0')
        return false;

    if (value_long <= 0 || value_long > MAX_BODY_SIZE)
    {
        DEBUG_MSG("Invalid client max body size : ", value);
        return false;
    }
    return true;
}

bool Parser::ipValidityChecker(std::string &ip)
{
    if (ip.empty())
        return false;

    int num_dots = 0;
    int segment_value = 0;
    int num_digits = 0;
    const char *ip_char = ip.c_str();
    while (*ip_char && (*ip_char == '.' || (*ip_char >= '0' && *ip_char <= '9')))
    {
        if (*ip_char == '.')
        {
            if (num_digits == 0 || segment_value > 255)
                return false;
            if (*(ip_char + 1) == '.')
                return false;
            num_dots++;
            segment_value = 0;
            num_digits = 0;
        }
        else if (*ip_char >= '0' && *ip_char <= '9')
        {
            segment_value = segment_value * 10 + (*ip_char - '0');
            num_digits++;
            if (num_digits > 3 || segment_value > 255)
                return false;
        }
        ip_char++;
    }
    if (num_digits == 0 || segment_value > 255)
        return false;

    return (num_dots == 3 && *ip_char == '\0');
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
    if (line.empty() || line[0] == '#')
        return EMPTY_LINE;
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
bool Parser::parseErrorBlock(std::istream &config_file, Server &server)
{
    std::string line, key, value;
    char *end; // To capture the position where parsing stops
    int error_code_long;

    while (std::getline(config_file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        if (line.find("[[server]]") != std::string::npos || line.find("[[server.error_page]]") != std::string::npos || line.find("[[server.location]]") != std::string::npos)
        {
            config_file.seekg(-static_cast<int>(line.length()) - 1, std::ios::cur);
            if (error_block_ok)
                return true;
            else
                return false;
        }
        ParseKeyValueResult result = checkKeyPair(line);
        if (result == KEY_VALUE_PAIR_WITH_QUOTES)
        {
            if (!parseKeyValue(line, key, value))
                return false;
            if (!key.empty() && !value.empty())
            {
                error_code_long = strtol(key.c_str(), &end, 10); // Base 10 conversion
                if (*end == '\0' && end != key.c_str() && (error_code_long > 0 && error_code_long < 1000))
                {
                    error_block_ok = true;
                    server.error_pages.insert(std::make_pair((int)error_code_long, value));
                }
                else
                {
                    error_block_ok = false;
                    return false;
                }
            }
        }
        error_block_ok = true;
    }
    return true;
}

bool Parser::parseServerBlock(std::istream &config_file, Server &server)
{
    std::string line;
    std::string key, value;

    while (std::getline(config_file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        if (line.find("[[server]]") != std::string::npos || line.find("[[server.error_page]]") != std::string::npos || line.find("[[server.location]]") != std::string::npos)
        {
            config_file.seekg(-static_cast<int>(line.length()) - 1, std::ios::cur);
            if (!server.port.empty() && ipValidityChecker(server.host) && !server.host.empty() && !server.root_directory.empty())
            {
                server_block_ok = true;
                return true;
            }
            return false;
        }
        if (!parseKeyValue(line, key, value))
            return false;
        if (key == "listen")
        {
            if (atoi(value.c_str()) > 0 && atoi(value.c_str()) < 65535)
                server.port = value;
            else
                DEBUG_MSG("Incorrect port: ", value);
        }
        if (key == "host")
        {
            if (ipValidityChecker(value))
                server.host = value;
            else
                DEBUG_MSG("Incorrect ip address: ", value);
        }
        if (key == "root")
            server.setRootDirectory(value);
        if (key == "index")
            server.index = value;
        if (key == "client_max_body_size")
        {
            if (checkMaxBodySize(value))
                server.client_max_body_size = atoi(value.c_str());
            else
            {
                throw std::runtime_error("Invalid client max body size: " + value);
                server.clear();
                return false;
            }
        }
    }
    return true;
}

std::vector<Server> Parser::parseConfig(const std::string config_file)
{
    std::vector<Server> servers_vector;
    std::string line;
    std::ifstream file(config_file.c_str());
    if (!file.is_open())
    {
        throw std::runtime_error("Error: can't open config file");
    }
    server_block_ok = false;
    error_block_ok = false;
    location_bloc_ok = false;
    new_server_found = false;
    Server new_server(0, "", "", "");
    DEBUG_MSG_1("Config file opened", "");

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        if (line.find("[[server]]") != std::string::npos)
        {
            new_server_found = true;
            parseServerBlock(file, new_server);
        }
        if (line.find("server.error_page]]") != std::string::npos)
        {
            parseErrorBlock(file, new_server);
        }
        if (line.find("server.location]]") != std::string::npos && server_block_ok)
        {
            parseLocationBlock(file, new_server);
        }
        if (server_block_ok && error_block_ok && location_bloc_ok && new_server_found)
        {
            servers_vector.push_back(new_server);
            new_server.clear();
            server_block_ok = false;
            error_block_ok = false;
            location_bloc_ok = false;
            new_server_found = false;
            DEBUG_MSG_1("Server setup completed", "");
        }
    }
    if (server_block_ok && error_block_ok && location_bloc_ok && new_server_found)
    {
        servers_vector.push_back(new_server);
        new_server.clear();
        server_block_ok = false;
        error_block_ok = false;
        location_bloc_ok = false;
        new_server_found = false;
        DEBUG_MSG_1("Server setup completed", "");
    }

    if (checkForDuplicates(servers_vector))
        throw std::runtime_error("Error: Duplicate server configuration found");
    if (servers_vector.empty())
        throw std::runtime_error("Error: No correctly configured servers found, please review configuration file");

    return servers_vector;
}


int Parser::checkForDuplicates(std::vector<Server> &servers_vector)
{
    for (size_t i = 0; i < servers_vector.size(); i++)
    {
        DEBUG_MSG_1("Checking for duplicates", "");
        for (size_t j = i + 1; j < servers_vector.size(); j++)
        {
            DEBUG_MSG_1("Checking for duplicates more", ""); 
            if (servers_vector[i].port == servers_vector[j].port && 
                servers_vector[i].host == servers_vector[j].host)
            {
                std::cerr << "Error: Duplicate server configuration found for " << servers_vector[i].host << ":" << servers_vector[i].port << std::endl;
                return 1;
            }
        }
    }
    return 0;
}   