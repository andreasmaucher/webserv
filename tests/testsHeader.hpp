#ifndef TESTS_HEADER_HPP
#define TESTS_HEADER_HPP

#include <iostream>
#include <fstream>
#include "../include/requestParser.hpp"
#include "../include/httpRequest.hpp"
#include "../include/httpResponse.hpp"
#include "../include/responseHandler.hpp"
#include "../include/server.hpp"

// Forward declare classes instead of including headers
class WebService;
class HttpRequest;

// testRequestParser.cpp
void test_request_parser_simple();
void test_request_parser_streaming();

// testResponseHandler.cpp
void test_responseHandler();
Server createFakeServerConfig();
HttpRequest createFakeHttpRequest(const std::string &method, const std::string &uri, const std::string &content_type, const std::string &content_length, const std::string &body);
void logTestResult(const std::string &testName, bool &passed, const std::string &result);
void run_test_(const std::string &test_name, const int expected_status_code, HttpRequest request, const Server &config);

// testMultiServer.cpp
std::vector<Server> createFakeServers();

#endif