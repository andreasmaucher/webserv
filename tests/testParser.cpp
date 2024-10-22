#include "testsHeader.hpp"

// void test_request_parser_streaming() {
//     HttpRequest request;
//     request.headers_parsed = false;
//     request.error_code = 0;

//     std::string raw_request_part1 =
//         "POST /upload HTTP/1.1\r\n"
//         "Host: localhost\r\n"
//         "Transfer-Encoding: chunked\r\n\r\n";

//     std::string raw_request_part2 =
//         "5\r\nHello\r\n"
//         "6\r\n World\r\n"
//         "40\r\nlalelilolulalelilolulalelilolulalelilozz\r\n"
//         "6\r\n World\r\n"
//         "0\r\n\r\n";

//     std::string raw_request = raw_request_part1 + raw_request_part2;

//     // Simulate recv in chunks
//     size_t position = 0;
//     size_t bytes_received;
//     bool request_complete = false;
//     size_t chunk_size = 70; // Simulate small chunks (you can adjust this)
    
//     while (!request_complete) {
//         // Simulate receiving a chunk of data
//         if (position < raw_request.size()) {
//             // Calculate the number of bytes to read (the size of the chunk)
//             bytes_received = std::min(chunk_size, raw_request.size() - position);

//             // Extract the chunk from the full raw request
//             std::string chunk = raw_request.substr(position, bytes_received);
//             position += bytes_received;

//             // Process the chunk
//             std::cout << "Received chunk: " << chunk << std::endl;

//             // Parse the chunk (pass the chunk directly to the parser)
//             request_complete = RequestParser::parseRawRequest(request, chunk, position);
//         } else {
//             break;  // Stop when we've simulated all the data
//         }
//     }

//     if (request_complete) {
//         std::cout << "Request parsing completed successfully.\n";
//         std::cout << "Method: " << request.method << "\n";
//         std::cout << "URI: " << request.uri << "\n";
//         std::cout << "Version: " << request.version << "\n";
//         std::cout << "Body: " << request.body << "\n";
//     } else {
//         std::cout << "Request parsing failed or incomplete.\n";
//     }
// }

void test_request_parser_simple() {

    HttpRequest request_simple;
    HttpRequest request_chunked;
    HttpRequest request_invalid;
    size_t position = 0;
    //ChunkState chunk_state;
    bool complete;
    request_simple.headers_parsed = false;
    request_chunked.headers_parsed = false;
    request_invalid.headers_parsed = false;
    request_simple.error_code = 0;
    request_chunked.error_code = 0;
    request_invalid.error_code = 0;

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
        std::cout << "Method: " << request_simple.method << "\n";
        std::cout << "URI: " << request_simple.uri << "\n";
        std::cout << "Version: " << request_simple.version << "\n";
        std::cout << "Body: " << request_simple.body << "\n";
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
        std::cout << "Method: " << request_chunked.method << "\n";
        std::cout << "URI: " << request_chunked.uri << "\n";
        std::cout << "Version: " << request_chunked.version << "\n";
        std::cout << "Body: " << request_chunked.body << "\n";
    } else {
        std::cout << "Request parsing failed.\n";
    }

    std::cout << "\n--> Testing invalid request:\n";
    position = 0;
    complete = RequestParser::parseRawRequest(request_invalid, invalid_request, position);

    if (complete) {
        std::cout << "Request parsing complete.\n";
        std::cout << "Method: " << request_invalid.method << "\n";
        std::cout << "URI: " << request_invalid.uri << "\n";
        std::cout << "Version: " << request_invalid.version << "\n";
        std::cout << "Body: " << request_invalid.body << "\n";
    } else {
        std::cout << "Request parsing failed.\n";
    }

    return;
}
