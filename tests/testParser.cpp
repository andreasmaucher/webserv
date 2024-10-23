#include "testsHeader.hpp"

//   //example requests:
//   const std::string raw_request =
//       "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: "
//       "Mozilla/5.0\r\n\r\n";
//   const std::string raw_request = 
//     "POST /submit-form HTTP/1.1\r\n"
//     "Host: www.example.com\r\n"
//     "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
//     "(KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36\r\n"
//     "Content-Type: application/x-www-form-urlencoded\r\n"
//     "Content-Length: 39\r\n"
//     "\r\n"
//     "name=John+Doe&email=johndoe%40email.com";
//    const std::string raw_request = "POST /upload HTTP/1.1\r\rHost: www.example.com\r\n"
//       "Transfer-Encoding: chunked\r\n"
//       "4\r\nWiki\r\n"
//       "5\r\npedia\r\n"
//       "0\r\n\r\n"

void test_request_parser_simple() {

    HttpRequest request_simple;
    HttpRequest request_chunked;
    HttpRequest request_invalid;
    size_t position = 0;
    //ChunkState chunk_state;
    bool complete;

    std::string simple_get_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: TestClient\r\n"
        "Accept: text/html\r\n"
        "\r\n";
    
    std::string post_chunked_request = 
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "4\r\n"
        "Wiki\r\n"
        "5\r\n"
        "pedia\r\n"
        "0\r\n"
        "\r\n";

    std::string invalid_request = 
        "GET / HTTP/1.1\r\n"
        "\r\n";

    std::cout << "--> Testing simple GET request:\n";
    complete = RequestParser::parseRawRequest(request_simple, simple_get_request, position);

    if (complete) {
        std::cout << "Request parsing complete.\n";
        request_simple.printRequest();
    } else {
        std::cout << "Request parsing failed.\n";
    }

    std::cout << "\n--> Testing POST with chunked transfer:\n";
    position = 0;
    while (true) {
        complete = RequestParser::parseRawRequest(request_chunked, post_chunked_request, position);
        if (complete || request_chunked.error_code != 0) {
            break;
        }
    }

    if (complete) {
        std::cout << "Request parsing complete.\n";
        request_chunked.printRequest();
    } else {
        std::cout << "Request parsing failed.\n";
    }

    std::cout << "\n--> Testing invalid request:\n";
    position = 0;
    complete = RequestParser::parseRawRequest(request_invalid, invalid_request, position);

    if (complete) {
        std::cout << "Request parsing complete.\n";
        request_invalid.printRequest();
    } else {
        std::cout << "Request parsing failed.\n";
    }

    return;
}

void test_request_parser_streaming() {
    HttpRequest request;

    std::string raw_request_part1 =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n\r\n";

    std::string raw_request_part2 =
        "5\r\nHello\r\n"
        "6\r\n World\r\n"
        "40\r\nlalelilolulalelilolulalelilolulalelilozz\r\n"
        "6\r\n World\r\n"
        "0\r\n\r\n";

    std::string raw_request = raw_request_part1;

    // Simulate recv in chunks
    size_t position = 0;
    size_t bytes_received;
    bool request_complete = false;
    size_t chunk_size = 70; // Simulate small chunks (you can adjust this)
    
    std::cout << "\n--> Testing streaming: " << std::endl;

    while (!request_complete) {
        // Simulate receiving a chunk of data
        if (position < raw_request.size()) {
            // Calculate the number of bytes to read (the size of the chunk)
            bytes_received = std::min(chunk_size, raw_request.size() - position);
            std::cout << "Bytes received: " << bytes_received << std::endl;
            // Extract the chunk from the full raw request
            std::string chunk = raw_request.substr(position, bytes_received);
            position += bytes_received;

            // Process the chunk
            std::cout << "Received chunk: " << chunk << std::endl;

            // Parse the chunk (pass the chunk directly to the parser)
            request_complete = RequestParser::parseRawRequest(request, chunk, position);
            if (!request_complete) {
                raw_request += raw_request_part2;  // Append the next part of the request
            }
        } else {
            break;  // Stop when we've simulated all the data
        }
    }

    if (request_complete) {
        std::cout << "Request parsing completed successfully.\n";
        std::cout << "Method: " << request.method << "\n";
        std::cout << "URI: " << request.uri << "\n";
        std::cout << "Version: " << request.version << "\n";
        std::cout << "Body: " << request.body << "\n";
    } else {
        std::cout << "Request parsing failed or incomplete.\n";
    }
}
