#!/usr/bin/env python3
import os
import sys
import cgi
import struct

def is_valid_png(data):
    # PNG files start with specific 8-byte signature
    png_signature = b'\x89PNG\r\n\x1a\n'
    return data.startswith(png_signature)

# Set up environment
server_root = os.environ.get('SERVER_ROOT', os.getcwd())
upload_dir = os.environ.get('UPLOAD_DIR', os.path.join(server_root, 'www/uploads'))

# Ensure upload directory exists
os.makedirs(upload_dir, exist_ok=True)

# Send response headers
print("Content-Type: text/html")
print()

print("""<!DOCTYPE html>
<html>
<head>
    <title>File Upload Result</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        .success { color: green; }
        .error { color: red; }
        a { display: inline-block; margin-top: 20px; }
    </style>
</head>
<body>
    <h1>File Upload Result</h1>
""")

try:
    # Parse the form data
    form = cgi.FieldStorage()
    
    if 'file' not in form:
        raise ValueError("No file was uploaded")
    
    fileitem = form['file']
    
    if not fileitem.filename:
        raise ValueError("No file was selected")
    
    # Get the filename and prepare the destination path
    filename = os.path.basename(fileitem.filename)
    filepath = os.path.join(upload_dir, filename)
    
    # Read file data
    file_data = fileitem.file.read()
    
    # For PNG files, verify the format
    if filename.lower().endswith(('.png', '.jpg', '.jpeg', '.gif')):
        if filename.lower().endswith('.png'):
            is_valid = is_valid_png(file_data)
            if not is_valid:
                raise ValueError("Invalid PNG file format")
    
    # Write file to disk in binary mode
    with open(filepath, 'wb') as f:
        f.write(file_data)
    
    # Success message
    print(f"<p class='success'>File <strong>{filename}</strong> was uploaded successfully!</p>")
    print(f"<p>Saved to: {filepath}</p>")
    print(f"<p>File size: {len(file_data)} bytes</p>")
    
    # For image files, display the image
    if filename.lower().endswith(('.png', '.jpg', '.jpeg', '.gif')):
        relative_path = f"/uploads/{filename}"
        print(f"<p><img src='{relative_path}' alt='{filename}' style='max-width: 500px;'></p>")
        print(f"<p>View image: <a href='{relative_path}' target='_blank'>{relative_path}</a></p>")
    
except Exception as e:
    print(f"<p class='error'>Error: {str(e)}</p>")

# Add a link back to the main page
print("""
    <a href="/">Back to Main Page</a>
</body>
</html>
""")