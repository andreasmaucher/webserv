#!/usr/bin/env python3
import sys
import os
import subprocess

# Debug information to stderr
# sys.stderr.write("Python CGI script starting...\n")
# sys.stderr.write(f"Current working directory: {os.getcwd()}\n")
# sys.stderr.write(f"Environment variables: {dict(os.environ)}\n")

# Print headers
print("Content-Type: text/plain")
print()  # Empty line to separate headers from body

# Run ls command and capture output
try:
    ls_output = subprocess.check_output(['ls', '-la'], text=True)
    print("Directory listing:")
    print(ls_output)
    # TODO: remove this line this is only for debugging, in the end only printout on stdout which is e.g postman
    #sys.stderr.write(f"ls output captured: {ls_output}\n")
except subprocess.CalledProcessError as e:
    # print("Error executing ls command:", e)
    sys.stderr.write(f"Error in ls command: {e}\n")

# sys.stderr.write("Python CGI script ending...\n")