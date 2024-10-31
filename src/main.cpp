
#include "http/requestParser.hpp"
#include "http/httpRequest.hpp"
#include "../tests/testsHeader.hpp"
#include "server/server.hpp"

// int main(int argc, char *argv[])
// {
//     (void)argc;
//     std::string port;
//     if (!argv[1])
//     {
//         std::cerr << "Usage: please ./a.out <port number, has to be over 1024>" <<std::endl;
//         port = PORT;
//         std::cerr << "Using default port, since no valid port was specified: " << port <<std::endl;
//     } else
//         port = argv[1];
//     try {
//         Server server(port);
//         server.start();
        
//     } catch (const std::exception& e) {
//         std::cerr << "Server error: " << e.what() <<std::endl;
//         return 1;
//     }
//     return (0);
// }


int main() {
    test_routing();
    // test_request_parser_simple();
    // test_request_parser_streaming();
    return 0;
}
