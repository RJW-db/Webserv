#!/bin/bash

# This script is used to run tests for the project.

if [[ "$(pwd)" != *"Webserv"* ]]; then
    echo "Error: You must run this script from within a Webserv directory."
    exit 1
fi

cd "$(pwd | sed 's|\(.*Webserv\).*|\1|')"


make
mkdir -p testing/results
rm -rf testing/results/*

# run with ./test.sh -y if you want to start the server
if [[ "$1" == "y" || "$1" == "Y" || "$1" == "-y" || "$1" == "-Y" ]]; then
    # Start the web server in the background with stdin from /dev/null
    # disable if you don't want server to start
    (./Webserv testing/test1.conf < /dev/null > testing/results/webservOut.txt 2>&1) &
    SERVER_PID=$!
    disown $SERVER_PID
    
    # Wait a moment and check if server started successfully
    sleep 2
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo "Error: Server failed to start. Check testing/results/webservOut.txt for details."
        cat testing/results/webservOut.txt
        exit 1
    fi
    echo "Server started with PID $SERVER_PID"
fi




cd testing

# Wait a moment for the server to start

echo "=== Testing Web Server ==="


./tests/post_tests.sh &
POST_PID=$!
./tests/get_tests.sh &
GET_PID=$!
./tests/get_fail_test.sh &
GET_FAIL_PID=$!
./tests/post_fail_test.sh &
POST_FAIL_PID=$!
./tests/chunked_tests.sh &
CHUNKED_PID=$!

echo "testing started, waiting for tests to finish..."

wait $POST_PID
wait $GET_PID
wait $GET_FAIL_PID
wait $POST_FAIL_PID
wait $CHUNKED_PID



echo "=== Tests Completed ==="
echo "Check the results in the 'results' directory. and summary.txt for details."

# # Clean up: Kill the server
# echo "Stopping server..."

if [[ -n "$SERVER_PID" ]]; then
    echo "Stopping server with PID $SERVER_PID..."
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
else
    echo "No server was started."
fi


echo server output in results/webservOut.txt

echo "Get results summary: "
cat results/get/summary.txt

echo -en "\n"

echo "Post results summary: "
cat results/post/summary.txt

echo -en "\n"

echo "Get fail results summary: "
cat results/get_fail/summary.txt
echo -en "\n"

echo "Post fail results summary: "
cat results/post_fail/summary.txt
echo -en "\n"

echo "Chunked results summary: "
cat results/chunked/summary.txt
