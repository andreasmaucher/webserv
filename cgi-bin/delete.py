#!/usr/bin/env python3
import sys
import os
import json

# Debug information to stderr
sys.stderr.write("Python DELETE CGI script starting...\n")

try:
    # Print headers first
    print("Content-Type: application/json")
    print()  # Empty line to separate headers from body

    # Get request information
    request_method = os.environ.get('REQUEST_METHOD', 'unknown')
    path_info = os.environ.get('PATH_INFO', '')
    query_string = os.environ.get('QUERY_STRING', '')

    # Check if it's actually a DELETE request
    if request_method != 'DELETE':
        response = {
            "status": "error",
            "message": f"Method {request_method} not allowed. This endpoint only accepts DELETE requests.",
            "code": 405  # Method Not Allowed
        }
    else:
        # Simulate deleting a resource
        response = {
            "status": "success",
            "message": "Resource deleted successfully",
            "details": {
                "method": request_method,
                "path_info": path_info,
                "query_string": query_string,
                "timestamp": "resource deleted at timestamp xyz"
            }
        }

    # Output JSON response
    print(json.dumps(response, indent=2))

except Exception as e:
    # If something goes wrong, send error response
    sys.stderr.write(f"Error in script: {str(e)}\n")
    error_response = {
        "status": "error",
        "message": str(e),
        "code": 500  # Internal Server Error
    }
    print(json.dumps(error_response, indent=2))

sys.stderr.write("Python DELETE CGI script ending...\n")