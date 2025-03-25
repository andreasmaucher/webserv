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
    // std::string port;
    std::string config_path;
    if (!argv[1])
    {
        std::cout << "Usage: ./webserv config_file\nUsing default configuration..." << std::endl;
        config_path = DEFAULT_CONFIG;
        // std::cerr << "Usage: please ./a.out <port number, has to be over 1024>" <<std::endl;
        // port = PORT;
        // std::cerr << "Using default port, since no valid port was specified: " << port <<std::endl;
    }
    else
    {
        config_path = argv[1];
        // port = argv[1];
    }
    try
    {
        WebService service(config_path);
        service.start();
        // Server server(port);
        // server.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}