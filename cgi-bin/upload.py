#!/usr/bin/env python3
import os
import sys
import cgi
import json

# Debug logging to stderr (will appear in server logs)
sys.stderr.write("Upload script starting...\n")

try:
    # Get the server's root directory from environment or use a default
    server_root = os.environ.get('DOCUMENT_ROOT', os.getcwd())
    upload_dir = os.path.join(server_root, "www", "uploads")
    
    sys.stderr.write(f"Server root: {server_root}\n")
    sys.stderr.write(f"Upload directory: {upload_dir}\n")

    # Create uploads directory if it doesn't exist
    os.makedirs(upload_dir, exist_ok=True)

    # Print headers first
    print("Content-Type: application/json")
    print()  # Empty line to separate headers from body

    # Parse the multipart form data
    form = cgi.FieldStorage()
    
    sys.stderr.write(f"Form fields: {list(form.keys())}\n")
    
    response = {
        "status": "success",
        "files": []
    }

    # Handle file upload
    if "file" in form:
        fileitem = form["file"]
        if fileitem.filename:
            # Get the filename
            filename = os.path.basename(fileitem.filename)
            filepath = os.path.join(upload_dir, filename)
            
            sys.stderr.write(f"Saving file to: {filepath}\n")
            
            # Save the file
            with open(filepath, 'wb') as f:
                f.write(fileitem.file.read())
            
            response["files"].append({
                "name": filename,
                "size": os.path.getsize(filepath),
                "path": filepath
            })
    else:
        response["status"] = "error"
        response["message"] = "No file field found in form data"

    print(json.dumps(response, indent=2))

except Exception as e:
    sys.stderr.write(f"Error in upload script: {str(e)}\n")
    print(json.dumps({
        "status": "error",
        "message": str(e)
    }, indent=2))

sys.stderr.write("Upload script ending...\n")