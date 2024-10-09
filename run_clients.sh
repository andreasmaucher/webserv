#!/bin/bash

# Launch multiple clients and each client will send a different message to the server.

# Log file to collect output messages
LOGFILE="client_log.txt"
> $LOGFILE  # Clear the log file at the start

# Array of messages to send
MESSAGES=("Hello from Client 1" "Hello from Client 2" "Hello from Client 3" "Hello from Client 4")

# Start the server in the background
./server 3490 &  # Run your server in the background
SERVER_PID=$!  # Store the server's process ID

# Run clients
for i in "${!MESSAGES[@]}"; do
    # Run each client with a different message
    (./client localhost "${MESSAGES[$i]}" >> $LOGFILE 2>&1) &
done

# Wait for all background processes to finish
wait

# Clean up: Kill the server process
kill $SERVER_PID

echo "All clients have finished. Check $LOGFILE for output."
