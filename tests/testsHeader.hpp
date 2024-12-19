#ifndef TESTSHEADER_HPP
#define TESTSHEADER_HPP

#include <iostream>
#include <fstream>
#include "../include/requestParser.hpp"
#include "../include/httpRequest.hpp"
#include "../include/httpResponse.hpp"
#include "../include/responseHandler.hpp"
#include "../include/server.hpp"

// testParser.cpp
void test_request_parser_simple();
void test_request_parser_streaming();

//testRouting.cpp
void test_responseHandler();
Server createFakeServerConfig();
HttpRequest createFakeHttpRequest(const std::string &method, const std::string &uri, const std::string &content_type, const std::string &content_length, const std::string &body);
void logTestResult(const std::string &testName, bool &passed, const std::string &result);
void run_test_(const std::string &test_name, const int expected_status_code, HttpRequest request, const Server &config);

#endif