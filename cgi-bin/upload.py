#!/usr/bin/env python3
import os
import sys
import cgi
import json

# Debug: Print environment variables
print("Content-Type: application/json")
print()  # Empty line to separate headers from body

try:
    # Create uploads directory if it doesn't exist
    upload_dir = "www/uploads"
    if not os.path.exists(upload_dir):
        os.makedirs(upload_dir)

    # Parse the multipart form data
    form = cgi.FieldStorage()
    
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
            
            # Save the file
            with open(filepath, 'wb') as f:
                f.write(fileitem.file.read())
            
            response["files"].append({
                "name": filename,
                "size": os.path.getsize(filepath),
                "path": filepath
            })

    print(json.dumps(response, indent=2))

except Exception as e:
    print(json.dumps({
        "status": "error",
        "message": str(e)
    }, indent=2))