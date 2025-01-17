#!/usr/bin/env python3
import sys
import os
import json
import urllib.parse

# Debug information to stderr
sys.stderr.write("Python Content-Type test script starting...\n")

try:
    # Get request information
    content_type = os.environ.get('CONTENT_TYPE', '')
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    request_method = os.environ.get('REQUEST_METHOD', '')
    
    # Print headers first
    print("Content-Type: application/json")
    print()  # Empty line to separate headers from body
    
    response = {
        "status": "success",
        "received_content_type": content_type,
        "received_content_length": content_length,
        "method": request_method,
        "data": None
    }

    # Handle different content types
    if content_length > 0:
        body = sys.stdin.read(content_length)
        
        if content_type == 'application/json':
            try:
                response["data"] = json.loads(body)
            except json.JSONDecodeError:
                response["data"] = "Invalid JSON received"
                
        elif content_type == 'application/x-www-form-urlencoded':
            form_data = urllib.parse.parse_qs(body)
            response["data"] = {k: v[0] for k, v in form_data.items()}
            
        elif content_type == 'text/plain':
            response["data"] = body
            
        elif content_type.startswith('multipart/form-data'):
            # For multipart, we need to use cgi.FieldStorage
            environ = os.environ.copy()
            environ['CONTENT_LENGTH'] = str(content_length)
            form = cgi.FieldStorage(
                fp=sys.stdin.buffer,
                environ=environ,
                keep_blank_values=True
            )
            
            form_data = {}
            for field in form.keys():
                if form[field].filename:
                    form_data[field] = {
                        "filename": form[field].filename,
                        "type": form[field].type,
                        "size": len(form[field].value)
                    }
                else:
                    form_data[field] = form[field].value
            response["data"] = form_data
        
        else:
            response["data"] = f"Unsupported Content-Type: {content_type}"

    print(json.dumps(response, indent=2))

except Exception as e:
    error_response = {
        "status": "error",
        "message": str(e)
    }
    print(json.dumps(error_response, indent=2))

sys.stderr.write("Python Content-Type test script ending...\n")