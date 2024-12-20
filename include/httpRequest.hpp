#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include "serverConfig.hpp"

struct ChunkState {
  size_t chunk_size;    // Size of the current chunk
  size_t bytes_read;    // Bytes of the current chunk that have been read
  bool in_chunk;        // Indicates whether we are in the middle of reading a chunk
  bool chunked_done;    // Indicates whether the chunked transfer is complete

  void reset() {
    chunk_size = 0;
    bytes_read = 0;
    in_chunk = false;
    chunked_done = false;
  }
  
  ChunkState() : chunk_size(0), bytes_read(0), in_chunk(false), chunked_done(false) {}
};

// Core data structure for incoming requests
class HttpRequest {
  public:
    HttpRequest();

    void reset();
    void printRequest();
      
    // // Getters
    // const std::string& getMethod() const { return method; }
    // const std::string& getUri() const { return uri; }
    // const std::string& getPath() const { return path; }
    // const std::string& getVersion() const { return version; }
    // const std::map<std::string, std::string>& getHeaders() const { return headers; }
    // const std::string& getBody() const { return body; }
    // const Route* getRoute() const { return route; }
    // const std::string& getFileName() const { return file_name; }
    // const std::string& getFileExtension() const { return file_extension; }
    // const std::string& getContentType() const { return content_type; }
    // bool isDirectory() const { return is_directory; }
    // bool isCgi() const { return is_cgi; }
    // int getErrorCode() const { return error_code; }
    // // Getters for ChunkState
    // size_t getChunkSize() const { return chunk_state.chunk_size; }
    // size_t getBytesRead() const { return chunk_state.bytes_read; }
    // bool isInChunk() const { return chunk_state.in_chunk; }
    // bool isChunkedDone() const { return chunk_state.chunked_done; }

    // // Setters
    // void setMethod(const std::string &m) { method = m; }
    // void setUri(const std::string &u) { uri = u; }
    // void setPath(const std::string &p) { path = p; }
    // void setVersion(const std::string &v) { version = v; }
    // void setHeader(const std::string &key, const std::string &value) { headers[key] = value; }
    // void setBody(const std::string &b) { body = b; }
    // void setRoute(const Route *r) { route = r; }
    // void setFileName(const std::string &name) { file_name = name; }
    // void setFileExtension(const std::string &ext) { file_extension = ext; }
    // void setContentType(const std::string &type) { content_type = type; }
    // void setIsDirectory(bool dir) { is_directory = dir; }
    // void setIsCgi(bool cgi) { is_cgi = cgi; }
    // void setErrorCode(int code) { error_code = code; }
    // // Setters for ChunkState
    // void setChunkSize(size_t &size) { chunk_state.chunk_size = size; }
    // void setBytesRead(size_t &bytes) { chunk_state.bytes_read = bytes; }
    // void setInChunk(bool &in_chunk) { chunk_state.in_chunk = in_chunk; }
    // void setChunkedDone(bool &chunked_done) { chunk_state.chunked_done = chunked_done; }

  //private:
    std::string raw_request;
    std::string method;                         // e.g., GET, POST
    std::string uri;                            // e.g., /index.html
    std::string path;                           // real path in server e.g., www/html/index.html
    std::string version;                        // e.g., HTTP/1.1
    std::map<std::string, std::string> headers; // e.g., Host, User-Agent
    std::string body; // The body of the request (optional, for POST/PUT)
    const Route *route;
    std::string file_name;
    std::string file_extension;
    std::string content_type;
    bool is_directory;
    bool is_cgi;
    int error_code;
    std::string queryString; //! ANDY should be included in uri
    std::string contentType; //! ANDY

    //for parsing
    size_t position;
    bool complete;
    bool headers_parsed;
    ChunkState chunk_state;
    int clientSocket; //! ANDY
  
};

#endif