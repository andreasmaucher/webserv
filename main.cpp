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
  const std::string rawRequest =
      "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: "
      "Mozilla/5.0\r\n\r\n";
  std::vector<std::string> requestLines = parser::splitIntoLines(rawRequest);
  std::vector<std::string>::iterator it = requestLines.begin();
  int i = 1;
  while (it != requestLines.end()) {
    std::cout << "LINE " << i << ": " << *it << std::endl;
    ++it;
    i++;
  }

  httpRequest request;
  it = requestLines.begin();

  parser::tokenizeRequestLine(*it, request); // passing 1st req line only
  std::cout << "Method: " << request.method << "\n"
            << "URI: " << request.uri << "\n"
            << "Version: " << request.version << std::endl;

  bool endOfHeaders = false;
  while (++it != requestLines.end() && !it->empty() && !endOfHeaders) {
    endOfHeaders = parser::tokenizeHeaders(*it, request);
  }
  std::cout << "Headers:\n";
  for (std::map<std::string, std::string>::iterator headerIter =
           request.headers.begin();
       headerIter != request.headers.end(); ++headerIter) {
    std::cout << "    " << headerIter->first << " " << headerIter->second
              << std::endl;
  }
  return 0;
}