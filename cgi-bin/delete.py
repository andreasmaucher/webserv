#!/usr/bin/env python3
import sys
import os
import json

#USER TESTING: curl -X DELETE -v http://localhost:8080/cgi-bin/delete.py/test.txt
# Enhanced debug logging
sys.stderr.write("Python DELETE CGI script starting...\n")
sys.stderr.write("All environment variables:\n")
for key, value in os.environ.items():
    sys.stderr.write(f"{key}: {value}\n")

sys.stderr.write(f"Current working directory: {os.getcwd()}\n")

try:
    # Print headers first
    print("Content-Type: application/json")
    print()  # Empty line to separate headers from body

    # Get request information
    request_method = os.environ.get('REQUEST_METHOD', 'unknown')
    path_info = os.environ.get('PATH_INFO', '')
    script_name = os.environ.get('SCRIPT_NAME', '')

    sys.stderr.write(f"Request Method: {request_method}\n")
    sys.stderr.write(f"Path Info: {path_info}\n")
    sys.stderr.write(f"Script Name: {script_name}\n")

    # Check if it's actually a DELETE request
    if request_method != 'DELETE':
        response = {
            "status": "error",
            "message": f"Method {request_method} not allowed. This endpoint only accepts DELETE requests.",
            "code": 405
        }
    else:
        # Extract filename from PATH_INFO
        if not path_info:
            response = {
                "status": "error",
                "message": "No file specified for deletion",
                "code": 400
            }
        else:
            # Remove leading slash if present
            filename = path_info.lstrip('/')
            file_path = os.path.join('www/uploads', filename)
            
            sys.stderr.write(f"Attempting to delete file: {file_path}\n")
            sys.stderr.write(f"File exists: {os.path.exists(file_path)}\n")
            
            if os.path.exists(file_path):
                try:
                    os.remove(file_path)
                    response = {
                        "status": "success",
                        "message": f"File {filename} deleted successfully",
                        "details": {
                            "method": request_method,
                            "path_info": path_info,
                            "deleted_file": filename,
                            "full_path": file_path
                        }
                    }
                except Exception as e:
                    response = {
                        "status": "error",
                        "message": f"Failed to delete file: {str(e)}",
                        "code": 500
                    }
            else:
                response = {
                    "status": "error",
                    "message": f"File {filename} not found at path: {file_path}",
                    "code": 404
                }

    print(json.dumps(response, indent=2))

except Exception as e:
    sys.stderr.write(f"Error in script: {str(e)}\n")
    error_response = {
        "status": "error",
        "message": str(e),
        "code": 500
    }
    print(json.dumps(error_response, indent=2))

sys.stderr.write("Python DELETE CGI script ending...\n")