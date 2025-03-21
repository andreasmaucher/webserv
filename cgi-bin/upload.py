#!/usr/bin/env python3
import os
import sys
import cgi
import json
import traceback
import re
import io
from datetime import datetime

#FOR TESTING UPLOAD FUNCTIONALITY: http://localhost:8080/static/upload.html

# Debug file setup
DEBUG_FILE = "/tmp/upload_debug.log"

def debug_log(message):
    with open(DEBUG_FILE, "a") as f:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"[{timestamp}] {message}\n")
    sys.stderr.write(f"{message}\n")

# File signature verification functions
def is_valid_png(data):
    png_signature = b'\x89PNG\r\n\x1a\n'
    return data.startswith(png_signature)

def is_valid_jpeg(data):
    # JPEG files start with FF D8 and end with FF D9
    return data.startswith(b'\xff\xd8') and (len(data) > 4)

def is_valid_gif(data):
    # GIF files start with either "GIF87a" or "GIF89a"
    return data.startswith(b'GIF87a') or data.startswith(b'GIF89a')

def validate_binary_file(filename, data):
    """Validate binary file based on its extension and content"""
    ext = os.path.splitext(filename.lower())[1]
    
    # Check if we have enough data to validate
    if len(data) < 8:
        debug_log(f"WARNING: File too small ({len(data)} bytes) to validate properly")
        return False
    
    # Validate based on file type
    if ext == '.png':
        valid = is_valid_png(data)
        debug_log(f"PNG validation result: {valid}")
        return valid
    elif ext in ('.jpg', '.jpeg'):
        valid = is_valid_jpeg(data)
        debug_log(f"JPEG validation result: {valid}")
        return valid
    elif ext == '.gif':
        valid = is_valid_gif(data)
        debug_log(f"GIF validation result: {valid}")
        return valid
    
    # For other binary formats, just check that we have data
    return len(data) > 0

def extract_file_from_multipart(environ):
    """Low-level extraction of file from multipart/form-data without cgi module"""
    try:
        # This is a fallback method if cgi.FieldStorage fails
        content_type = environ.get('CONTENT_TYPE', '')
        if not content_type.startswith('multipart/form-data'):
            debug_log("Not a multipart/form-data request")
            return None, None
        
        # Extract boundary
        boundary_match = re.search(r'boundary=([^;]+)', content_type)
        if not boundary_match:
            debug_log("No boundary found in Content-Type")
            return None, None
        
        boundary = boundary_match.group(1).strip()
        if boundary.startswith('"') and boundary.endswith('"'):
            boundary = boundary[1:-1]
        
        boundary = f'--{boundary}'
        debug_log(f"Using boundary: {boundary}")
        
        # Read raw POST data
        content_length = int(environ.get('CONTENT_LENGTH', 0))
        post_data = sys.stdin.buffer.read(content_length)
        debug_log(f"Read {len(post_data)} bytes of raw POST data")
        
        # Split by boundary
        parts = post_data.split(boundary.encode('utf-8'))
        
        # Look for file part
        filename = None
        file_data = None
        
        for part in parts:
            # Skip empty parts and final boundary
            if not part or part.strip() == b'--':
                continue
            
            # Split headers and content
            try:
                headers_end = part.find(b'\r\n\r\n')
                if headers_end == -1:
                    continue
                
                headers = part[:headers_end].decode('utf-8', errors='ignore')
                content = part[headers_end+4:]
                
                # Check if this part is a file
                if 'filename=' in headers:
                    # Extract filename
                    filename_match = re.search(r'filename="([^"]+)"', headers)
                    if filename_match:
                        filename = filename_match.group(1)
                        debug_log(f"Found file part with filename: {filename}")
                        
                        # Last 2 bytes might be \r\n, remove them if present
                        if content.endswith(b'\r\n'):
                            content = content[:-2]
                        
                        file_data = content
                        break
            except Exception as e:
                debug_log(f"Error processing part: {str(e)}")
                continue
        
        return filename, file_data
    
    except Exception as e:
        debug_log(f"Error in extract_file_from_multipart: {str(e)}")
        debug_log(traceback.format_exc())
        return None, None

# Log script start
debug_log("=== UPLOAD.PY STARTING ===")
debug_log(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'Not set')}")
debug_log(f"CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'Not set')}")
debug_log(f"CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'Not set')}")

try:
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

    response = {
        "status": "success",
        "files": []
    }

    # Try standard cgi.FieldStorage first
    debug_log("Attempting to parse form data with cgi.FieldStorage...")
    try:
        form = cgi.FieldStorage(
            keep_blank_values=True,
            strict_parsing=False,
            encoding='utf-8'
        )
        
        debug_log(f"Form parsed. Fields: {list(form.keys())}")
        
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
                
                # Detect if this is a binary file
                ext = os.path.splitext(filename.lower())[1]
                is_binary = ext in ('.png', '.jpg', '.jpeg', '.gif', '.bmp', '.pdf', '.zip', '.mp3', '.mp4')
                debug_log(f"File type: {'binary' if is_binary else 'text'}")
                
                # Read the file data completely before writing
                file_data = fileitem.file.read()
                debug_log(f"Read {len(file_data)} bytes from uploaded file")
                
                # Validate binary files
                if is_binary and not validate_binary_file(filename, file_data):
                    debug_log("Binary file validation failed")
                    print(json.dumps({
                        "status": "error",
                        "message": f"Invalid {ext[1:].upper()} file. The uploaded file does not have a valid signature."
                    }))
                    sys.exit(0)
                
                # Save file in the appropriate mode
                with open(filepath, 'wb') as f:
                    f.write(file_data)
                
                file_size = os.path.getsize(filepath)
                debug_log(f"File saved. Size on disk: {file_size} bytes")
                
                response["files"].append({
                    "name": filename,
                    "size": file_size,
                    "path": f"/uploads/{filename}"
                })
            else:
                debug_log("File item has no filename")
                
                # Fall back to manual extraction
                debug_log("Trying manual multipart extraction...")
                filename, file_data = extract_file_from_multipart(os.environ)
                
                if filename and file_data:
                    debug_log(f"Manual extraction successful: {filename} ({len(file_data)} bytes)")
                    
                    filepath = os.path.join(upload_dir, os.path.basename(filename))
                    
                    # Detect if this is a binary file
                    ext = os.path.splitext(filename.lower())[1]
                    is_binary = ext in ('.png', '.jpg', '.jpeg', '.gif', '.bmp', '.pdf', '.zip', '.mp3', '.mp4')
                    
                    # Validate binary files if possible
                    if is_binary and not validate_binary_file(filename, file_data):
                        debug_log("Binary file validation failed during manual extraction")
                        print(json.dumps({
                            "status": "error",
                            "message": f"Invalid {ext[1:].upper()} file. The uploaded file does not have a valid signature."
                        }))
                        sys.exit(0)
                    
                    # Write file
                    with open(filepath, 'wb') as f:
                        f.write(file_data)
                    
                    file_size = os.path.getsize(filepath)
                    debug_log(f"File saved via manual extraction. Size on disk: {file_size} bytes")
                    
                    response["files"].append({
                        "name": os.path.basename(filename),
                        "size": file_size,
                        "path": f"/uploads/{os.path.basename(filename)}"
                    })
                else:
                    response["status"] = "error"
                    response["message"] = "No valid file found in form data"
        else:
            debug_log("No 'file' field in form data")
            
            # Try manual extraction as a fallback
            debug_log("Trying manual multipart extraction...")
            filename, file_data = extract_file_from_multipart(os.environ)
            
            if filename and file_data:
                debug_log(f"Manual extraction successful: {filename} ({len(file_data)} bytes)")
                
                filepath = os.path.join(upload_dir, os.path.basename(filename))
                
                # Save file
                with open(filepath, 'wb') as f:
                    f.write(file_data)
                
                file_size = os.path.getsize(filepath)
                debug_log(f"File saved via manual extraction. Size on disk: {file_size} bytes")
                
                response["files"].append({
                    "name": os.path.basename(filename),
                    "size": file_size,
                    "path": f"/uploads/{os.path.basename(filename)}"
                })
            else:
                response["status"] = "error"
                response["message"] = "No file field found in form data"
    
    except Exception as form_error:
        debug_log(f"Error with standard form processing: {str(form_error)}")
        debug_log(traceback.format_exc())
        
        # Fall back to manual extraction
        debug_log("Trying manual multipart extraction after standard form processing failed...")
        filename, file_data = extract_file_from_multipart(os.environ)
        
        if filename and file_data:
            debug_log(f"Manual extraction successful: {filename} ({len(file_data)} bytes)")
            
            filepath = os.path.join(upload_dir, os.path.basename(filename))
            
            # Save file
            with open(filepath, 'wb') as f:
                f.write(file_data)
            
            file_size = os.path.getsize(filepath)
            debug_log(f"File saved via manual extraction. Size on disk: {file_size} bytes")
            
            response["files"].append({
                "name": os.path.basename(filename),
                "size": file_size,
                "path": f"/uploads/{os.path.basename(filename)}"
            })
        else:
            response["status"] = "error"
            response["message"] = f"Form processing error: {str(form_error)}"

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

# Add this function to debug binary content
def debug_binary_content(data, filename):
    """Log information about the binary content"""
    debug_log(f"Binary content length: {len(data)} bytes")
    
    # Check first few bytes
    if len(data) > 16:
        hex_bytes = ' '.join(f'{b:02x}' for b in data[:16])
        debug_log(f"First 16 bytes: {hex_bytes}")
    
    # Check for common image signatures
    if filename.lower().endswith('.png'):
        png_signature = b'\x89PNG\r\n\x1a\n'
        if data.startswith(png_signature):
            debug_log("PNG signature detected: Valid")
        else:
            debug_log("PNG signature missing: INVALID")
            hex_bytes = ' '.join(f'{b:02x}' for b in data[:8])
            debug_log(f"Expected: 89 50 4E 47 0D 0A 1A 0A, Got: {hex_bytes}")
    
    elif filename.lower().endswith(('.jpg', '.jpeg')):
        if data.startswith(b'\xff\xd8'):
            debug_log("JPEG signature detected: Valid")
        else:
            debug_log("JPEG signature missing: INVALID")
    
    # Add file size check
    if request.headers.get('Content-Length'):
        expected_size = int(request.headers.get('Content-Length'))
        actual_size = len(data)
        percent_received = (actual_size / expected_size) * 100
        debug_log(f"Received {actual_size} of {expected_size} bytes ({percent_received:.1f}%)")