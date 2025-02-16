#!/usr/bin/python3

import os
import sys
import time
from datetime import datetime
import cgi

def main():
    # Get parameters from both methods
    path_info = os.environ.get('PATH_INFO', '').lstrip('/')  # Remove leading slash
    query_params = cgi.FieldStorage()
    
    # Get filename from either PATH_INFO or query parameter
    filename = path_info if path_info else query_params.getvalue('filename', '')
    
    # If the file is in uploads directory, prepend the path
    if filename and not os.path.isabs(filename):
        filename = os.path.join('www/uploads', filename)
    
    # Get current timestamp
    current_time = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    
    print("Content-Type: text/plain\r\n")
    
    if not filename:
        print(f"Current timestamp: {current_time}")
        return
        
    if not os.path.exists(filename):
        print(f"Error: File not found: {filename}")
        return
        
    # Get just the filename without the path for display
    display_name = os.path.basename(filename)
    name, ext = os.path.splitext(display_name)
    new_filename = f"{name}_{current_time}{ext}"
    
    print(f"Original filename: {display_name}")
    print(f"Timestamped filename: {new_filename}")

if __name__ == "__main__":
    main()