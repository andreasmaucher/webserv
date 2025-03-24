#!/usr/bin/env python3

# TESTING COMMAND: curl http://localhost:8080/cgi-bin/timestamp.py/flames.jpeg

import os
import sys
import cgi
import cgitb
from datetime import datetime

# Enable CGI traceback for debugging
cgitb.enable()

print("Content-Type: text/plain\n")

# Debug: Print environment variables
print("--- Environment Variables ---")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'NOT SET')}")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'NOT SET')}")
print(f"CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'NOT SET')}")
print(f"PATH_INFO: {os.environ.get('PATH_INFO', 'NOT SET')}")
print("---------------------------\n")

try:
    # First try to get filename from query parameter
    query_string = os.environ.get('QUERY_STRING', '')
    print("Raw query string: " + query_string)
    
    # Remove leading '?' if present
    if query_string.startswith('?'):
        query_string = query_string[1:]
    print("Cleaned query string: " + query_string)
    
    # Manual parsing
    params = {}
    if query_string:
        for param in query_string.split('&'):
            if '=' in param:
                key, value = param.split('=', 1)
                params[key] = value
    
    filename = params.get('filename')
    
    # If no filename in query params, check PATH_INFO
    if not filename:
        path_info = os.environ.get('PATH_INFO', '')
        if path_info:
            # Remove leading slash if present
            filename = path_info.lstrip('/')
            print("Filename from PATH_INFO: " + filename)
    
    print("Final filename: " + str(filename))
    
    if not filename:
        print("Error: No filename specified")
        print("Please use either ?filename=example.txt in the URL or /example.txt after the script name")
        sys.exit(0)
    
    # Check if file exists in uploads directory
    upload_dir = os.path.join(os.getcwd(), "www", "uploads")
    file_path = os.path.join(upload_dir, filename)
    
    print("Looking for file at: " + file_path)

    if not os.path.exists(file_path):
        print("Error: File " + filename + " not found in uploads directory")
        print("Current working directory: " + os.getcwd())
        print("Files in upload dir: " + str(os.listdir(upload_dir) if os.path.exists(upload_dir) else "directory not found"))
        sys.exit(0)
    
    # Get file timestamps
    file_creation_time = os.path.getctime(file_path)
    file_modification_time = os.path.getmtime(file_path)
    file_size = os.path.getsize(file_path)

    # Convert timestamps to readable format
    creation_time_str = datetime.fromtimestamp(file_creation_time).strftime("%Y-%m-%d %H:%M:%S")
    modification_time_str = datetime.fromtimestamp(file_modification_time).strftime("%Y-%m-%d %H:%M:%S")

    # Print the file information
    print("File: " + filename)
    print("Size: " + str(file_size) + " bytes")
    print("Creation timestamp: " + creation_time_str)
    print("Last modified: " + modification_time_str)
    
except Exception as e:
    print("Error: " + str(e))