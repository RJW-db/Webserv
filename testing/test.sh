#!/bin/bash

# This script is used to run tests for the project.

cd ../


make
# Start the web server in the background
# disable if you don't want server to start
# ./Webserv testing/test1.conf > /dev/null &
SERVER_PID=$!

cd testing

# Wait a moment for the server to start
sleep 2

echo "=== Testing Web Server ==="

rm -rf results/*
mkdir -p results/upload1 results/upload2 results/upload3

# Create results directory if it doesn't exist

# # Test 1: Basic GET request to port 8080
# echo "1. Testing GET / request to localhost:15000/"
# curl -i http://localhost:15000/ > results/get1.txt &
# echo -e "\n"

# # Test 2: GET request for PDF file
# echo "2. Testing GET request for PDF file to localhost:15000/test.pdf with Host header 'server3'"
# curl -i -H "Host: server3" http://localhost:15000/test.pdf > results/get2.txt &
# echo -e "\n"

# # Test 3: GET request for a non-existent file
# echo "3. Testing GET request for non-existent file to localhost:15000/nonexistent"
# curl -i http://localhost:15000/nonexistent > results/get3.txt &
# echo -e "\n"

# # Test 4: GET request for JavaScript file with different host and port
# echo "4. Testing GET request for JS file to localhost:15001/main.js with Host header 'jsserver'"
# curl -i -H "Host: server2" http://localhost:15001/main.js > results/get4.txt &
# echo -e "\n"

# # Test 5: GET request for XML file
# echo "5. Testing GET request for XML file to localhost:15001/catalog.xml with Host header 'server2'"
# curl -i -H "Host: server2" http://localhost:15001/catalog.xml > results/get5.txt &
# echo -e "\n"

# # test 6: POST request to upload a file
# echo "6. Testing POST request to upload test1.jpg to localhost:15000/upload"
# curl -i -X POST -H "Expect:" -H "Host: notexistinghost" -F "myfile=@expectedResults/upload1/test1.jpg" http://localhost:15000/upload1 > results/post1.txt &
# echo -e "\n"

# # Test 7: POST request to upload PNG file to server2
# echo "7. Testing POST request to upload test2.png to localhost:15001/upload with Host header 'server2'"
# curl -i -X POST -H "Expect:" -H "Host: server2" -F "myfile=@expectedResults/upload2/test2.png" http://localhost:15001/upload2 > results/post2.txt & 
# echo -e "\n"

# # Test 8: POST request to upload small text file to server3
# echo "8. Testing POST request to upload small.txt to localhost:15000/upload with Host header 'server1'"
# curl -i -X POST -H "Expect:" -H "Host: server1" -F "myfile=@expectedResults/upload1/small.txt" http://localhost:15000/upload1 > results/post3.txt &
# echo -e "\n"

# Test 9: POST request to upload large text file to server3
echo "9. Testing POST request to upload 1M.txt to localhost:15000/upload with Host header 'server3' which should fail due to size limit"
curl -i -X POST -H "Expect:" -H "Host: server3" -F "myfile=@expectedResults/upload3/1M.txt" http://localhost:15000/upload3 > results/post4.txt &
echo -e "\n"

# # Test 10: POST request to upload large text file to server1
# echo "10. Testing POST request to upload 1M.txt to localhost:15000/upload with Host header 'server1'"
# curl -i -X POST -H "Expect:" -H "Host: server1" -F "myfile=@expectedResults/upload3/1M.txt" http://localhost:15000/upload1 > results/post5.txt 
# echo -e "\n"

sleep 0.5
# # Clean up: Kill the server
# echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

exit 0