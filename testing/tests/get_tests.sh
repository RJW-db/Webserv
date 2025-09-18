#!/bin/bash

if [[ "$(pwd)" != *"Webserv"* ]]; then
    echo "Error: You must run this script from within a Webserv directory."
    exit 1
fi

cd "$(pwd | sed 's|\(.*Webserv\).*|\1|')/testing"

mkdir -p results/get


# Pre-requests to create cookies on the server
echo "Creating cookies on server..."
curl -s -i -H "Connection: close" http://localhost:15000/ > results/get/cookie_request.txt
sleep 0.5
# Extract session_id from the Set-Cookie header in the response
session_id=$(grep -i "Set-Cookie:" results/get/cookie_request.txt | grep -o 'session_id=[^;]*' | cut -d= -f2)
if [[ -z "$session_id" ]]; then
    echo "Error: session_id not found in cookie response."
    exit 1
fi
echo "Extracted session_id: $session_id"

# Test 1: Basic GET request to port 8080
echo "1. Testing GET / request to localhost:15000/"
curl -s -i -H "Connection: close" -H "Cookie: session_id=$session_id" http://localhost:15000/ > results/get/get1.txt &
echo -e "\n"

# Test 2: GET request for PDF file
echo "2. Testing GET request for PDF file to localhost:15000/test.pdf with Host header 'server3'"
curl -s -i -H "Connection: close" -H "Host: server3" -H "Cookie: session_id=$session_id" http://localhost:15000/test.pdf > results/get/get2.txt &
echo -e "\n"

# Test 3: GET request for JavaScript file with different host and port
echo "3. Testing GET request for JS file to localhost:15001/main.js with Host header 'jsserver'"
curl -s -i -H "Connection: close" -H "Host: server2" -H "Cookie: session_id=$session_id" http://localhost:15001/main.js > results/get/get3.txt &
echo -e "\n"

# Test 4: GET request for XML file with percent-encoded path
echo "4. Testing GET request for XML file to localhost:15001/catalog.xml with Host header 'server2'"
curl -s -i -H "Connection: close" -H "Host: server2" -H "Cookie: session_id=$session_id" http://localhost:15001/catalog.xml > results/get/get4.txt &
echo -e "\n"

wait
sleep 0.1


{
cmp -s expectedResults/get/get1.txt results/get/get1.txt && echo "test 1 completed successfully" || echo "test 1 failed because there is difference in expected output"
cmp -s expectedResults/get/get2.txt results/get/get2.txt && echo "test 2 completed successfully" || echo "test 2 failed because there is difference in expected output"
cmp -s expectedResults/get/get3.txt results/get/get3.txt && echo "test 3 completed successfully" || echo "test 3 failed because there is difference in expected output"
cmp -s expectedResults/get/get4.txt results/get/get4.txt && echo "test 4 completed successfully" || echo "test 4 failed because there is difference in expected output"
} >> results/get/summary.txt 2>&1

echo "Get tests completed"
exit 0