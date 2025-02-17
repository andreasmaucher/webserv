#!/usr/bin/env python3

import os
import sys
import time
from datetime import datetime

print("Content-Type: text/plain")
print()  # Blank line to separate headers from body

try:
    # Extract the filename from PATH_INFO
    path_info = os.environ.get('PATH_INFO', '').lstrip('/')
    if not path_info:
        print("Error: No filename provided")
        sys.exit(1)

    # Construct full path to the file
    file_path = os.path.join('www/uploads', path_info)
    
    # Get file's modification time
    if os.path.exists(file_path):
        file_timestamp = os.path.getmtime(file_path)
        timestamp_str = datetime.fromtimestamp(file_timestamp).strftime('%Y-%m-%d_%H-%M-%S')
        
        # Get original filename without path
        original_filename = os.path.basename(path_info)
        
        # Create new filename with actual file timestamp
        filename_without_ext, file_extension = os.path.splitext(original_filename)
        new_filename = f"{filename_without_ext}_{timestamp_str}{file_extension}"
        
        print(f"Original filename: {original_filename}")
        print(f"Timestamped filename: {new_filename}")
        print(f"File's actual modification time: {datetime.fromtimestamp(file_timestamp)}")
    else:
        print(f"Error: File {path_info} not found in uploads directory")
        sys.exit(1)

except Exception as e:
    print(f"Error: {str(e)}")
    sys.exit(1)