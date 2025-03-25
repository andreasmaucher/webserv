#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include "server.hpp"

struct ChunkState
{
  size_t chunk_size; // Size of the current chunk
  size_t bytes_read; // Bytes of the current chunk that have been read
  bool in_chunk;     // Indicates whether we are in the middle of reading a chunk
  bool chunked_done; // Indicates whether the chunked transfer is complete

  void reset()
  {
    chunk_size = 0;
    bytes_read = 0;
    in_chunk = false;
    chunked_done = false;
  }

  ChunkState() : chunk_size(0), bytes_read(0), in_chunk(false), chunked_done(false) {}
};

// Core data structure for incoming requests
class HttpRequest
{
public:
  HttpRequest();

  void reset();
  void printRequest();

  // private:
  std::string raw_request;
  std::string method;                         // e.g., GET, POST
  std::string uri;                            // e.g., /index.html
  std::string path;                           // real path in server e.g., www/html/index.html
  std::string version;                        // e.g., HTTP/1.1
  std::map<std::string, std::string> headers; // e.g., Host, User-Agent
  std::string body;                           // The body of the request (optional, for POST/PUT)
  const Route *route;
  std::string file_name;
  std::string file_extension;
  std::string content_type;
  bool is_directory;
  bool is_cgi;
  int error_code;
  std::string queryString;
  std::string contentType;

  // for parsing
  size_t position;
  bool complete;
  bool headers_parsed;
  ChunkState chunk_state;
  int clientSocket;
  bool client_closed_connection; // Set to true when recv() returns 0
};

#endif