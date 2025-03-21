#!/usr/bin/env python3

# This script has an execution error, not a syntax error
# The script can be parsed correctly but will fail during execution

print("Content-Type: text/html")
print("")

# This will execute but then cause a runtime error
print("<h1>About to crash...</h1>")

# This will cause a division by zero error
result = 1 / 0

print("This will never be displayed")