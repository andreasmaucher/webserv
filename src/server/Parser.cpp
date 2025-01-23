/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizhakov <mrizhakov@student.42.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/24 14:17:32 by mrizakov          #+#    #+#             */
/*   Updated: 2025/01/23 18:26:45 by mrizhakov        ###   ########.fr       */
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
            server.setRoute(route.uri, route);
            config_file.seekg(-static_cast<int>(line.length()) - 1, std::ios::cur);
            return true;
        }
        if (line.find("[[server.location]]") != std::string::npos)
        {
            server.setRoute(route.uri, route);
            server.debugPrintRoutes();
            route = Route();
            continue;
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
        location_bloc_ok = true;
    }
    server.setRoute(route.uri, route);
    server.debugPrintRoutes();
    return true;
}

// bool Parser::checkMaxBodySize(const std::string &value)
// {
//     if (is)

//     (atoi(value) >)

// }

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
            return true;
        }
        ParseKeyValueResult result = checkKeyPair(line);
        if (result == KEY_VALUE_PAIR_WITH_QUOTES)
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
        // TODO: check which values are mandatory before giving ok
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
            return true;
        if (key == "listen" && atoi(value.c_str()) > 0 && atoi(value.c_str()) < 65535)
        {
            server.port = value;
        }
        else
            std::cerr << "Incorrect host ip address: " << value << std::endl;
        if (key == "host")
        {
            if (ipValidityChecker(value))
                server.host = value;
            else
                std::cerr << "Port : " << value << " is incorrect" << std::endl;
        }
        if (key == "root")
            server.setRootDirectory(value);
        if (key == "index")
            server.index = value;
        if (key == "client_max_body_size")
            server.client_max_body_size = value;
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
        std::cerr << "Error: can't open config file" << std::endl;
        return servers_vector;
    }
    server_block_ok = false;
    error_block_ok = false;
    location_bloc_ok = false;
    new_server_found = false;
    Server new_server(0, "", "", "");
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
        // std::cout << "---------->> server_block_ok: " << server_block_ok << std::endl;
        // std::cout << "---------->> error_block_ok: " << error_block_ok << std::endl;
        // std::cout << "---------->> location_bloc_ok: " << location_bloc_ok << std::endl;
        // std::cout << "---------->> new_server_found: " << new_server_found << std::endl;

        if (server_block_ok && error_block_ok && location_bloc_ok && new_server_found)
        {
            servers_vector.push_back(new_server);
            new_server.clear();
            server_block_ok = false;
            error_block_ok = false;
            location_bloc_ok = false;
            new_server_found = false;
        }
    }
    // std::cout << "---------->> server_block_ok: " << server_block_ok << std::endl;
    // std::cout << "---------->> error_block_ok: " << error_block_ok << std::endl;
    // std::cout << "---------->> location_bloc_ok: " << location_bloc_ok << std::endl;
    // std::cout << "---------->> new_server_found: " << new_server_found << std::endl;
    if (server_block_ok && error_block_ok && location_bloc_ok && new_server_found)
    {
        servers_vector.push_back(new_server);
        new_server.clear();
        server_block_ok = false;
        error_block_ok = false;
        location_bloc_ok = false;
        new_server_found = false;
    }
    return servers_vector;
}
