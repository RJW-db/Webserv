#!/bin/bash

# Script to send manual text request to webserver
# Usage: ./send_manual_text.sh [port] [host]

PORT=${1:-8080}
HOST=${2:-localhost}
REQUEST_FILE="manual_text_request.txt"

echo "Sending manual text request to $HOST:$PORT..."
echo "Using request file: $REQUEST_FILE"
echo "---"

# Send the raw HTTP request using netcat
cat "$REQUEST_FILE" | nc "$HOST" "$PORT"

echo
echo "Request sent!"