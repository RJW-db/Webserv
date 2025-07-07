#!/bin/bash

# This script is used to run tests for the project.

cd ../


make
# Start the web server in the background
# disable if you don't want server to start


cd testing

rm -rf results/*
# Wait a moment for the server to start

echo "=== Testing Web Server ==="


./tests/post_tests.sh &
./tests/get_tests.sh &

echo "testing started, waiting for tests to finish..."

sleep 3

echo "=== Tests Completed ==="
echo "Check the results in the 'results' directory. and summary.txt for details."

# # Clean up: Kill the server
# echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null


echo "Get results summary: "
cat results/get/summary.txt
echo -en "\n"

echo "Post results summary: "
cat results/post/summary.txt