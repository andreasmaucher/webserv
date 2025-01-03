#include "../include/server.hpp"
#include "../include/serverConfig.hpp"
#include "../include/httpRequest.hpp"
#include "../include/requestParser.hpp"
#include "../include/httpResponse.hpp"
#include "../include/responseHandler.hpp"
#include "../include/mimeTypeMapper.hpp"
#include "../tests/testsHeader.hpp"

int main(int argc, char *argv[])
{
    (void)argc;
    std::string port = PORT;
    if (!argv[1])
    {
        std::cerr << "Usage: please ./webserv <config_file>" <<std::endl;
        port = PORT;
        std::cerr << "Using default config file, since no config file was specified: " << port <<std::endl;
    } else
        // port = argv[1];
    try {
        Server server(port, argv[1]);
        server.start();
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() <<std::endl;
        return 1;
    }
    return (0);
}