#!/bin/bash

# Script to send manual text request to webserver with proper \r\n line endings
# Usage: ./send_manual_text.sh [port] [host]

PORT=${1:-8080}
HOST=${2:-localhost}
REQUEST_FILE="manual_text_request.txt"

echo "Sending manual text request to $HOST:$PORT..."
echo "Using request file: $REQUEST_FILE"
echo "Converting line endings to \\r\\n format..."
echo "---"

# Convert Unix line endings to Windows/HTTP line endings (\r\n) and send
sed 's/$/\r/' "$REQUEST_FILE" | nc "$HOST" "$PORT"

echo
echo "Request sent!"