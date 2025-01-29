#!/usr/bin/env python3
import sys
import os
import json

# Debug information to stderr
sys.stderr.write("Python POST CGI script starting...\n")
sys.stderr.write(f"Script PID: {os.getpid()}\n")

try:
    # Print headers first (always needed)
    print("Content-Type: application/json")
    print()  # Empty line to separate headers from body

    # Get the content length from environment variable
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    
    sys.stderr.write(f"Content-Length: {content_length}\n")
    sys.stderr.write("Starting to read POST data...\n")
    
    # Read POST data from stdin
    post_data = sys.stdin.read(content_length) if content_length > 0 else ""
    
    sys.stderr.write(f"Finished reading POST data. Length: {len(post_data)}\n")
    sys.stderr.write(f"Received data: {post_data}\n")
    
    # Create response object
    response = {
        "status": "success",
        "message": "Data received",
        "received_data": post_data,
        "env_vars": {
            "CONTENT_LENGTH": os.environ.get('CONTENT_LENGTH', 'not set'),
            "REQUEST_METHOD": os.environ.get('REQUEST_METHOD', 'not set'),
            "CONTENT_TYPE": os.environ.get('CONTENT_TYPE', 'not set'),
            "QUERY_STRING": os.environ.get('QUERY_STRING', 'not set')
        }
    }
    
    sys.stderr.write("Preparing to send response...\n")
    # Output JSON response
    print(json.dumps(response, indent=2))
    sys.stderr.write("Response sent.\n")

except Exception as e:
    sys.stderr.write(f"Error in script: {str(e)}\n")
    print("Content-Type: application/json")
    print()
    error_response = {
        "status": "error",
        "message": str(e)
    }
    print(json.dumps(error_response, indent=2))

sys.stderr.write("Python POST CGI script ending...\n")