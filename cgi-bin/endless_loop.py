#!/usr/bin/env python3
import sys
import os
import subprocess
import time


# DEBUG information to stderr
sys.stderr.write("Python CGI script starting...\n")
sys.stderr.write(f"Current working directory: {os.getcwd()}\n")
sys.stderr.write(f"Environment variables: {dict(os.environ)}\n")

# Print headers
print("Content-Type: text/plain")
print()  # Empty line to separate headers from body

# Endless loop to test non-blocking and process killing
while True:
    sys.stdout.write("Still running...\n")
    time.sleep(1)
    # sys.stderr.write("Work, baby work!...\n")
# Run ls command and capture output


sys.stderr.write("Python CGI script ending...\n")