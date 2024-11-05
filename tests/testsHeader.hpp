#ifndef TESTSHEADER_HPP
#define TESTSHEADER_HPP

#include <iostream>
#include "../include/requestParser.hpp"
#include "../include/httpRequest.hpp"
#include "../include/httpResponse.hpp"
#include "../include/responseHandler.hpp"
#include "../include/serverConfig.hpp"

// testParser.cpp
void test_request_parser_simple();
void test_request_parser_streaming();

//testRouting.cpp
void test_routing();
HttpRequest createFakeHttpRequest();
ServerConfig createFakeServerConfig();

#endif