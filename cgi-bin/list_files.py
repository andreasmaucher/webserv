#!/usr/bin/env python3
import os
import json
import sys

# Skip printing Content-Type altogether - let your web server handle it
# Just output the raw JSON data

try:
    # Get the uploads directory path
    uploads_dir = os.path.join(os.getcwd(), "www", "uploads")
    
    # Log to stderr for debugging (this won't be part of the output)
    sys.stderr.write(f"Looking for files in: {uploads_dir}\n")
    
    # List all files in the directory
    files = []
    if os.path.exists(uploads_dir) and os.path.isdir(uploads_dir):
        for file in os.listdir(uploads_dir):
            file_path = os.path.join(uploads_dir, file)
            # Skip directories, index.html, and hidden files
            if os.path.isfile(file_path) and file != "index.html" and not file.startswith('.'):
                files.append(file)
        sys.stderr.write(f"Found {len(files)} files\n")
    else:
        sys.stderr.write(f"Directory not found: {uploads_dir}\n")
    
    # Write ONLY the JSON output - no headers at all
    print(json.dumps(files))
    
except Exception as e:
    sys.stderr.write(f"Error in list_files.py: {str(e)}\n")
    print(json.dumps({"error": str(e)}))