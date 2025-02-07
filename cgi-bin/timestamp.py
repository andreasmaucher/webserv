#!/usr/bin/env python3
import sys
import os
import time

# Write headers first
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")

# Get PATH_INFO
path_info = os.environ.get('PATH_INFO', '')

if path_info:
    # Remove leading slash if present
    if path_info.startswith('/'):
        path_info = path_info[1:]
    
    file_path = os.path.join('www/uploads', path_info)
    
    if os.path.exists(file_path):
        # Get file's modification time
        timestamp = time.strftime("%Y-%m-%d_%H-%M-%S", time.localtime(os.path.getmtime(file_path)))
        sys.stdout.write(f"File: {path_info}\n")
        sys.stdout.write(f"Last modified: {timestamp}\n")
    else:
        sys.stdout.write(f"Error: File not found: {path_info}\n")
else:
    # No file specified, show current time
    timestamp = time.strftime("%Y-%m-%d_%H-%M-%S", time.localtime())
    sys.stdout.write(f"Current timestamp: {timestamp}\n")