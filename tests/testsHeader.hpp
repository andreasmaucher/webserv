#ifndef TESTSHEADER_HPP
#define TESTSHEADER_HPP

#include "../src/http/requestParser.hpp"
#include "../src/http/httpRequest.hpp"
#include "../src/http/httpResponse.hpp"
#include "../src/http/responseHandler.hpp"
#include "../src/server/serverConfig.hpp"
#include <iostream>

// testParser.cpp
void test_request_parser_simple();
void test_request_parser_streaming();

//testRouting.cpp
void test_routing();
HttpRequest createFakeHttpRequest();
ServerConfig createFakeServerConfig();

#endif