#!/usr/bin/env python3
import sys
import os
import json

# Debug information to stderr
sys.stderr.write("Python POST CGI script starting...\n")
sys.stderr.write(f"Script PID: {os.getpid()}\n")

try:
    # Print headers first (always needed)
    sys.stdout.write("Content-Type: application/json\r\n")
    sys.stdout.write("\r\n")  # Empty line to separate headers from body

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
    sys.stdout.write(json.dumps(response, indent=2))
    sys.stdout.flush()
    sys.stderr.write("Response sent.\n")

except Exception as e:
    sys.stderr.write(f"Error in script: {str(e)}\n")
    sys.stdout.write("Content-Type: application/json\r\n")
    sys.stdout.write("\r\n")
    error_response = {
        "status": "error",
        "message": str(e)
    }
    sys.stdout.write(json.dumps(error_response, indent=2))
    sys.stdout.flush()

sys.stderr.write("Python POST CGI script ending...\n")