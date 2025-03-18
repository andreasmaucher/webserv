#!/usr/bin/env python3
import os
import sys
import cgi
import json
import traceback
from datetime import datetime

#FOR TESTING UPLOAD FUNCTIONALITY: http://localhost:8080/static/upload.html

# Debug file setup
DEBUG_FILE = "/tmp/upload_debug.log"

def debug_log(message):
    with open(DEBUG_FILE, "a") as f:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"[{timestamp}] {message}\n")
    sys.stderr.write(f"{message}\n")

# PNG signature verification
def is_valid_png(data):
    png_signature = b'\x89PNG\r\n\x1a\n'
    return data.startswith(png_signature)

# Log script start
debug_log("=== UPLOAD.PY STARTING ===")
debug_log(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'Not set')}")
debug_log(f"CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'Not set')}")
debug_log(f"CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'Not set')}")

try:
    # Configure cgi to handle larger uploads and disable field storage limits
    cgi.maxlen = 20 * 1024 * 1024  # 20MB max
    
    # Get the server's root directory from environment or use current directory
    server_root = os.environ.get('DOCUMENT_ROOT', os.getcwd())
    upload_dir = os.path.join(server_root, "www", "uploads")
    
    debug_log(f"Server root: {server_root}")
    debug_log(f"Upload directory: {upload_dir}")

    # Create uploads directory if it doesn't exist
    os.makedirs(upload_dir, exist_ok=True)
    
    # Print headers first
    print("Content-Type: application/json")
    print()  # Empty line to separate headers from body

    # Only parse form data for POST requests
    if os.environ.get('REQUEST_METHOD') != 'POST':
        debug_log("ERROR: Not a POST request")
        print(json.dumps({
            "status": "error",
            "message": "Only POST requests are supported"
        }))
        sys.exit(0)

    # Parse the multipart form data with proper configuration
    debug_log("About to parse form data...")
    form = cgi.FieldStorage(
        keep_blank_values=True,
        strict_parsing=False,
        encoding='utf-8'
    )
    
    debug_log(f"Form parsed. Fields: {list(form.keys())}")
    
    response = {
        "status": "success",
        "files": []
    }

    # Handle file upload
    if "file" in form:
        fileitem = form["file"]
        debug_log(f"File item type: {type(fileitem)}")
        debug_log(f"Has filename: {hasattr(fileitem, 'filename')}")
        
        if hasattr(fileitem, 'filename') and fileitem.filename:
            # Get the filename
            filename = os.path.basename(fileitem.filename)
            filepath = os.path.join(upload_dir, filename)
            
            debug_log(f"Filename: {filename}")
            debug_log(f"Saving file to: {filepath}")
            
            # Read the file data completely before writing
            try:
                # IMPORTANT: Read binary data in binary mode
                file_data = fileitem.file.read()
                debug_log(f"Read {len(file_data)} bytes from uploaded file")
                
                # For PNG files, verify the signature
                if filename.lower().endswith('.png'):
                    debug_log(f"PNG file detected. Checking signature...")
                    if len(file_data) >= 8:
                        if is_valid_png(file_data):
                            debug_log("Valid PNG signature detected")
                        else:
                            debug_log("WARNING: Invalid PNG signature!")
                            debug_log(f"First 16 bytes: {file_data[:16].hex()}")
                            
                            # Reject the upload if it's not a valid PNG
                            debug_log("Rejecting invalid PNG file")
                            print(json.dumps({
                                "status": "error",
                                "message": "Invalid PNG file detected. The uploaded file does not have a valid PNG signature."
                            }))
                            sys.exit(0)
                    else:
                        debug_log("WARNING: File too small to be a valid PNG")
                        print(json.dumps({
                            "status": "error",
                            "message": "File too small to be a valid PNG"
                        }))
                        sys.exit(0)
                
                # CRITICAL: Write in binary mode with 'wb'
                with open(filepath, 'wb') as f:
                    f.write(file_data)
                
                file_size = os.path.getsize(filepath)
                debug_log(f"File saved. Size on disk: {file_size} bytes")
                
                # For PNG files, verify the saved file
                if filename.lower().endswith('.png'):
                    with open(filepath, 'rb') as f:
                        check_data = f.read(8)
                        if is_valid_png(check_data):
                            debug_log("Written PNG file verified successfully")
                        else:
                            debug_log("ERROR: Written PNG file is corrupted!")
                            os.remove(filepath)  # Remove corrupted file
                            print(json.dumps({
                                "status": "error",
                                "message": "PNG file was corrupted during upload. Please try again."
                            }))
                            sys.exit(0)
                
                response["files"].append({
                    "name": filename,
                    "size": file_size,
                    "path": f"/uploads/{filename}"
                })
            except Exception as file_error:
                debug_log(f"Error processing file: {str(file_error)}")
                debug_log(traceback.format_exc())
                response["status"] = "error"
                response["message"] = f"File processing error: {str(file_error)}"
        else:
            debug_log("File item has no filename")
            response["status"] = "error"
            response["message"] = "No filename provided"
    else:
        debug_log("No 'file' field in form data")
        response["status"] = "error"
        response["message"] = "No file field found in form data"

    # Output the JSON response
    print(json.dumps(response, indent=2))

except Exception as e:
    debug_log(f"Error in upload script: {str(e)}")
    debug_log(traceback.format_exc())
    print(json.dumps({
        "status": "error",
        "message": str(e)
    }, indent=2))

debug_log("Upload script ending...")