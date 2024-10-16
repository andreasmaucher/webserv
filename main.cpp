#include "requestParser.hpp"
#include "httpRequest.hpp"

int main() {
  // example request(the real one will be passed from the socket code as 1 long line)
  // const std::string raw_request =
  //     "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: "
  //     "Mozilla/5.0\r\n\r\n";
  const std::string raw_request = 
    "POST /submit-form HTTP/1.1\r\n"
    "Host: www.example.com\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 39\r\n"
    "\r\n"
    "name=John+Doe&email=johndoe%40email.com";
  
  HttpRequest request;

  try {

    if (raw_request.empty()) {
      request.error_code = 400; //400 BAD_REQUEST
      throw std::runtime_error("Empty request");
    }
    std::vector<std::string> request_lines = RequestParser::splitIntoLines(raw_request);

    //debugging print
    std::vector<std::string>::iterator it = request_lines.begin();
    int i = 0;
    while (it != request_lines.end()) {
      std::cout << "LINE " << ++i << ": " << *it << std::endl;
      ++it;
    }

    it = request_lines.begin();

    RequestParser::tokenizeRequestLine(*it, request); // passing 1st request line only
    //debugging print
    std::cout << "Method: " << request.method << "\n"
              << "URI: " << request.uri << "\n"
              << "Version: " << request.version << std::endl;

    RequestParser::tokenizeHeaders(it, request_lines, request);
    //debugging print
    std::cout << "Headers:\n";
    for (std::map<std::string, std::string>::iterator header_iter = request.headers.begin(); header_iter != request.headers.end(); ++header_iter) {
      std::cout << "    " << header_iter->first << " " << header_iter->second << std::endl;
    }

    RequestParser::saveValidBody(it, request_lines, request);
    //debugging print
    std::cout << "Body: " << request.body << std::endl;
    
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "HTTP Error Code: " << request.error_code << std::endl;

    return request.error_code; // handle in calling func to create appropriate response
  }

  return 0;
}