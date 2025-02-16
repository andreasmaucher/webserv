#!/usr/bin/env python3
import os
import sys

# Print headers (required)
print("Content-Type: text/plain")
print()  # Empty line is required!

# Print debugging info
print("=== Script Location ===")
print(f"Script path: {os.path.realpath(__file__)}")
print(f"Working directory: {os.getcwd()}")

print("\n=== Environment Variables ===")
for key, value in sorted(os.environ.items()):
    print(f"{key}: {value}")

print("\n=== Request Info ===")
print(f"Query string: {os.environ.get('QUERY_STRING', 'None')}")

# Print all environment variables
print("\n=== CGI Environment Variables ===")
for key, value in sorted(os.environ.items()):
    print(f"{key}: {value}")

# Print additional request information
print("\n=== Standard Input (POST data) ===")
if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    post_data = sys.stdin.read(content_length)
    print(post_data)
else:
    print("No POST data (GET request)")