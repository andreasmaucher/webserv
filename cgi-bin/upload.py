#!/usr/bin/env python3
import os
import sys
import cgi
import json

# Debug logging to stderr (will appear in server logs)
sys.stderr.write("Upload script starting...\n")
sys.stderr.write(f"Content-Type: {os.environ.get('CONTENT_TYPE', 'Not set')}\n")
sys.stderr.write(f"Content-Length: {os.environ.get('CONTENT_LENGTH', 'Not set')}\n")

try:
    # Configure cgi to use files for large uploads
    cgi.maxlen = 10 * 1024 * 1024  # 10MB max
    cgi.valid_boundary = lambda x: True  # Accept any boundary

    # Parse the multipart form data
    form = cgi.FieldStorage(keep_blank_values=True)
    
    sys.stderr.write(f"Form keys: {list(form.keys())}\n")
    
    # Print headers first
    print("Content-Type: application/json")
    print()  # Empty line to separate headers from body

    response = {
        "status": "success",
        "files": []
    }

    if "file" not in form:
        sys.stderr.write("No 'file' field in form data\n")
        response["status"] = "error"
        response["message"] = "No file field found in form data"
        print(json.dumps(response, indent=2))
        sys.exit(0)

    fileitem = form["file"]
    if not fileitem.filename:
        sys.stderr.write("File field present but no filename\n")
        response["status"] = "error"
        response["message"] = "No filename provided"
        print(json.dumps(response, indent=2))
        sys.exit(0)

    # Get the filename and save
    filename = os.path.basename(fileitem.filename)
    upload_dir = os.path.join(os.getcwd(), "www", "uploads")
    os.makedirs(upload_dir, exist_ok=True)
    
    filepath = os.path.join(upload_dir, filename)
    sys.stderr.write(f"Saving file to: {filepath}\n")
    
    with open(filepath, 'wb') as f:
        f.write(fileitem.file.read())
    
    response["files"].append({
        "name": filename,
        "size": os.path.getsize(filepath),
        "path": f"/uploads/{filename}"
    })

    print(json.dumps(response, indent=2))

except Exception as e:
    sys.stderr.write(f"Error in upload script: {str(e)}\n")
    print(json.dumps({
        "status": "error",
        "message": str(e)
    }, indent=2))

sys.stderr.write("Upload script ending...\n")