#!/usr/bin/env python3
import sys
import os
import subprocess

# Debug information to stderr
sys.stderr.write("Python CGI script starting...\n")

# Print headers
print("Content-Type: text/plain")
print()  # Empty line to separate headers from body

# Run ls command and capture output
try:
    ls_output = subprocess.check_output(['ls', '-la'], text=True)
    print("Directory listing:")
    print(ls_output)
except subprocess.CalledProcessError as e:
    print("Error executing ls command:", e)

sys.stderr.write("Python CGI script ending...\n")