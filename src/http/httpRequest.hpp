#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

struct ChunkState {
    size_t chunk_size;    // Size of the current chunk
    size_t bytes_read;    // Bytes of the current chunk that have been read
    bool in_chunk;        // Indicates whether we are in the middle of reading a chunk
    bool chunked_done;    // Indicates whether the chunked transfer is complete

  ChunkState() : chunk_size(0), bytes_read(0), in_chunk(false), chunked_done(false) {}
};

class HttpRequest {
public:
  std::string method;                         // e.g., GET, POST
  std::string uri;                            // e.g., /index.html
  std::string version;                        // e.g., HTTP/1.1
  std::map<std::string, std::string> headers; // e.g., Host, User-Agent
  std::string body; // The body of the request (optional, for POST/PUT)
  
  int error_code;
  bool headers_parsed;
  ChunkState chunk_state;

  HttpRequest();
  void printRequest();
  
};

#endif