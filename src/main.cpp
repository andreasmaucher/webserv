#include "http/requestParser.hpp"
#include "http/httpRequest.hpp"
#include "../tests/testsHeader.hpp"

int main() {
    test_request_parser_simple();
    test_request_parser_streaming();
    return 0;
}
