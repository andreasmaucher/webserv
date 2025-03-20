#!/usr/bin/env python3
import os
import sys
import cgi
import time

# Parse query parameters first
form = cgi.FieldStorage()
error_type = form.getvalue("type", "none")

# Start with proper CGI headers
print("Content-Type: text/html")
print()  # Empty line is required to separate headers from body

# Handle different error types
if error_type == "500":
    # Generate a 500 error
    print("Status: 500 Internal Server Error")
    print("<html><body><h1>500 Internal Server Error</h1><p>This is a deliberate 500 error.</p></body></html>")
    sys.exit(0)
    
elif error_type == "timeout":
    # Display a message first
    print("<html><body><h1>Timeout Test</h1><p>The script will now sleep for 30 seconds...</p></body></html>")
    sys.stdout.flush()  # Make sure the message is sent before sleeping
    
    # Sleep to test timeout
    time.sleep(30)
    print("If you see this, your server didn't time out after 30 seconds.")
    sys.exit(0)
    
elif error_type == "crash":
    # Show a message before crashing
    print("<html><body><h1>Crash Test</h1><p>The script will now crash deliberately...</p></body></html>")
    sys.stdout.flush()
    
    # Cause a crash
    1/0  # Division by zero error
    
elif error_type == "large":
    # Generate a large response
    print("<html><body><h1>Large Response Test</h1>")
    print("<p>Generating a 10MB response...</p>")
    
    for i in range(100000):
        print(f"<div>Line {i} of test data</div>")
        if i % 1000 == 0:
            sys.stdout.flush()
    
    print("</body></html>")
    
elif error_type == "malformed":
    # Send malformed output (not proper CGI)
    print("This is not proper CGI output")
    print("Because it's missing the Content-Type header")
    sys.exit(0)
    
elif error_type == "slow":
    # Send data slowly
    print("<html><body><h1>Slow Response Test</h1>")
    print("<p>Sending data slowly, one character at a time...</p>")
    print("<div>")
    sys.stdout.flush()
    
    for i in range(30):
        sys.stdout.write(".")
        sys.stdout.flush()
        time.sleep(1)
    
    print("</div><p>Slow response complete!</p></body></html>")
    
elif error_type == "env":
    # Show environment variables
    print("<html><body><h1>CGI Environment Variables</h1>")
    print("<pre>")
    for key, value in sorted(os.environ.items()):
        print(f"{key}: {value}")
    print("</pre></body></html>")
    
else:
    # Display the main menu interface
    print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Error Test Tool</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        h1 { color: #d9534f; }
        .card {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
            box-shadow: 0 4px 8px rgba(0,0,0,0.1);
        }
        .card h2 { margin-top: 0; color: #333; }
        .card p { color: #666; }
        .btn {
            display: inline-block;
            padding: 10px 15px;
            background-color: #007bff;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            font-weight: bold;
            margin-top: 10px;
        }
        .btn:hover { background-color: #0056b3; }
        .warning { background-color: #fff3cd; border-color: #ffeeba; }
        .note { font-size: 0.9em; font-style: italic; margin-top: 5px; }
    </style>
</head>
<body>
    <h1>CGI Error Testing Tool</h1>
    <p>Click on any test below to trigger the specific error. Watch how your web server handles each error case.</p>
    
    <div class="card">
        <h2>500 Internal Server Error</h2>
        <p>Tests if your server correctly handles when a CGI script returns a 500 status code.</p>
        <a href="?type=500" class="btn">Trigger 500 Error</a>
    </div>
    
    <div class="card warning">
        <h2>Timeout Error</h2>
        <p>This test will run for 30 seconds or until your server times out. Your browser will appear to be loading.</p>
        <a href="?type=timeout" class="btn">Trigger Timeout</a>
        <p class="note">Note: You may need to restart your server if it doesn't handle the timeout properly.</p>
    </div>
    
    <div class="card">
        <h2>Script Crash</h2>
        <p>This will cause the script to crash with an unhandled exception. Your server should detect this and return an error.</p>
        <a href="?type=crash" class="btn">Crash Script</a>
    </div>
    
    <div class="card warning">
        <h2>Large Response</h2>
        <p>Generates a 10MB response to test how your server handles large amounts of data.</p>
        <a href="?type=large" class="btn">Generate Large Response</a>
        <p class="note">Warning: This might cause your browser to become slow.</p>
    </div>
    
    <div class="card">
        <h2>Malformed Output</h2>
        <p>Sends an invalid CGI response with improper headers.</p>
        <a href="?type=malformed" class="btn">Send Bad Output</a>
    </div>
    
    <div class="card">
        <h2>Slow Response</h2>
        <p>Sends data very slowly (one dot per second for 30 seconds).</p>
        <a href="?type=slow" class="btn">Slow Response</a>
    </div>
    
    <div class="card">
        <h2>Environment Variables</h2>
        <p>Shows all environment variables your server is passing to CGI scripts.</p>
        <a href="?type=env" class="btn">Show Environment</a>
    </div>
    
    <hr>
    <p>This tool helps you test your web server's error handling capabilities. A robust server should gracefully handle all these error cases.</p>
</body>
</html>""")

# Make sure to flush the output
sys.stdout.flush()