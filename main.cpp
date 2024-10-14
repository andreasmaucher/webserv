// #include <iostream>

// int main(int ac, char **av)
// {
//     (void)av;
//     if (ac < 1 || ac > 2)
//     {
//         std::cout << "Invalid number of arguments" << std::endl;
//         return (1);
//     }
// }

#include "parser.hpp"

int main() {
  // example request(the real one will be passed from the socket code as 1 long
  // line). Print is for debug
  // const std::string rawRequest =
  //     "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: "
  //     "Mozilla/5.0\r\n\r\n";
  const std::string rawRequest = 
    "POST /submit-form HTTP/1.1\r\n"
    "Host: www.example.com\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 38\r\n"
    "\r\n"
    "name=John+Doe&email=johndoe%40email.com";

  std::vector<std::string> requestLines = parser::splitIntoLines(rawRequest);
  std::vector<std::string>::iterator it = requestLines.begin();
  int i = 0;
  while (it != requestLines.end()) {
    std::cout << "LINE " << ++i << ": " << *it << std::endl;
    ++it;
  }

  httpRequest request;
  it = requestLines.begin();

  parser::tokenizeRequestLine(*it, request); // passing 1st request line only
  std::cout << "Method: " << request.method << "\n"
            << "URI: " << request.uri << "\n"
            << "Version: " << request.version << std::endl;

  parser::tokenizeHeaders(it, requestLines, request);
  std::cout << "Headers:\n";
  for (std::map<std::string, std::string>::iterator headerIter =
           request.headers.begin();
       headerIter != request.headers.end(); ++headerIter) {
    std::cout << "    " << headerIter->first << " " << headerIter->second
              << std::endl;
  }

  parser::saveBody(it, requestLines, request);
  std::cout << "Body: " << request.body << std::endl;
 
  return 0;
}