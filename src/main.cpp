#include "http/requestParser.hpp"
#include "http/httpRequest.hpp"
#include "../tests/testsHeader.hpp"
#include "server/server.hpp"



int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: please ./a.out <port number, has to be over 1024>" <<std::endl;
        // printf("Usage: please ./a.out <port number, has to be over 1024>\n");
        return(1);
    }
    try {
        Server server(argv[1]);
        server.start();
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() <<std::endl;
        return 1;
    }
    return (0);
}


// int main() {
//     test_request_parser_simple();
//     test_request_parser_streaming();
//     return 0;
// }

// int main() {

//   HttpRequest request;
//   HttpResponse response;

//   std::string raw_request;
//   char buffer[4096];
//   size_t bytes_received;
//   size_t position = 0;
//   bool request_complete = false;

//   while (!request_complete) {
//       bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
      
//       if (bytes_received < 0) {
//         // Error in receiving data
//         perror("recv failed");
//         close(socket_fd);
//         return;
//       } else if (bytes_received == 0) {
//         // 499 NGNIX custom code for client closed connection => next step: close socket
//         // request should be fully received at this point unless client changed/closed page before sending full request
//         close(socket_fd);
//         return;
//       }
      
//       buffer[bytes_received] = '\0';
//       raw_request += std::string(buffer, bytes_received); //append buffer to raw_request
      
//       // Check if we have reached the end of headers to proceed parsing
//       if (raw_request.find("\r\n\r\n") != std::string::npos) {
//         request_complete = RequestParser::parseRawRequest(request, raw_request, position);

//       }
//   }

//   processRequest(request, response); // process request, check for errors and populate response

//   sendResponse(socket_fd, response); // send response to client

//   if (request.headers["Connection"] == "keep-alive" && request.version == "HTTP/1.1") {
//     // Keep the socket open for future requests
//   } else {
//     // Close the connection if Connection: close or HTTP/1.0
//     close(socket_fd);
//   }

//   // // example requests:
//   // // const std::string raw_request =
//   // //     "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: "
//   // //     "Mozilla/5.0\r\n\r\n";
//   // const std::string raw_request = 
//   //   "POST /submit-form HTTP/1.1\r\n"
//   //   "Host: www.example.com\r\n"
//   //   "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
//   //   "(KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36\r\n"
//   //   "Content-Type: application/x-www-form-urlencoded\r\n"
//   //   "Content-Length: 39\r\n"
//   //   "\r\n"
//   //   "name=John+Doe&email=johndoe%40email.com";
//   //  const std::string raw_request = "POST /upload HTTP/1.1\r\rHost: www.example.com\r\n"
//   //     "Transfer-Encoding: chunked\r\n"
//   //     "4\r\nWiki\r\n"
//   //     "5\r\npedia\r\n"
//   //     "0\r\n\r\n"
// }