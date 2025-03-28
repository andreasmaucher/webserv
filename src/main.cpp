/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mrizakov <mrizakov@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/28 14:08:20 by mrizakov          #+#    #+#             */
/*   Updated: 2025/03/28 14:08:43 by mrizakov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/webService.hpp"
#include "../include/server.hpp"
#include "../include/httpRequest.hpp"
#include "../include/requestParser.hpp"
#include "../include/httpResponse.hpp"
#include "../include/responseHandler.hpp"
#include "../include/mimeTypeMapper.hpp"
#include "../include/cgi.hpp"

int main(int argc, char *argv[])
{
    signal(SIGCHLD, SIG_IGN);

    (void)argc;
    std::string config_path;
    if (!argv[1])
    {
        std::cout << "Usage: ./webserv config_file\nUsing default configuration..." << std::endl;
        config_path = DEFAULT_CONFIG;
    }
    else
    {
        config_path = argv[1];
    }
    try
    {
        WebService service(config_path);
        service.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}