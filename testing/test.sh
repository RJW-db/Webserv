#!/bin/bash

# This script is used to run tests for the project.

cd ../


make
mkdir -p testing/results
rm -rf testing/results/*


read -p "Do you want to start the web server? (y/n): " start_server
if [[ "$start_server" == "y" || "$start_server" == "Y" ]]; then
    # Start the web server in the background
# disable if you don't want server to start
./Webserv testing/test1.conf > testing/results/webservOut.txt &
SERVER_PID=$!
sleep 2  # Wait a moment for the server to start
fi



cd testing

# Wait a moment for the server to start

echo "=== Testing Web Server ==="


./tests/post_tests.sh &
POST_PID=$!
./tests/get_tests.sh &
GET_PID=$!

echo "testing started, waiting for tests to finish..."

wait $POST_PID
wait $GET_PID



echo "=== Tests Completed ==="
echo "Check the results in the 'results' directory. and summary.txt for details."

# # Clean up: Kill the server
# echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo server output in results/webservOut.txt

echo "Get results summary: "
cat results/get/summary.txt
echo -en "\n"

echo "Post results summary: "
cat results/post/summary.txt


