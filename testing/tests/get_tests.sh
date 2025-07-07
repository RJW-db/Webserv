#!/bin/bash

cd /home/saleunin/Desktop/Webserv/testing 

mkdir -p results/get
# Test 1: Basic GET request to port 8080
echo "1. Testing GET / request to localhost:15000/"
curl -i -H "Connection: close" http://localhost:15000/ > results/get/get1.txt &
echo -e "\n"

# Test 2: GET request for PDF file
echo "2. Testing GET request for PDF file to localhost:15000/test.pdf with Host header 'server3'"
curl -i -H "Connection: close" -H "Host: server3" http://localhost:15000/test.pdf > results/get/get2.txt &
echo -e "\n"

# Test 3: GET request for JavaScript file with different host and port
echo "3. Testing GET request for JS file to localhost:15001/main.js with Host header 'jsserver'"
curl -i -H "Connection: close" -H "Host: server2" http://localhost:15001/main.js > results/get/get3.txt &
echo -e "\n"

# Test 3: GET request for XML file
echo "4. Testing GET request for XML file to localhost:15001/catalog.xml with Host header 'server2'"
curl -i -H "Connection: close" -H "Host: server2" http://localhost:15001/catalog.xml > results/get/get4.txt &
echo -e "\n"

# Test 5: GET request for a non-existent file
echo "5. Testing GET request for non-existent file to localhost:15000/nonexistent"
curl -i -H "Connection: close" http://localhost:15000/nonexistent > results/get/get5.txt &
echo -e "\n"

sleep 1
{
cmp -s expectedResults/get/get1.txt results/get/get1.txt && echo "test 1 completed successfully" || echo "test 1 failed because there is difference in expected output"
cmp -s expectedResults/get/get2.txt results/get/get2.txt && echo "test 2 completed successfully" || echo "test 2 failed because there is difference in expected output"
cmp -s expectedResults/get/get3.txt results/get/get3.txt && echo "test 3 completed successfully" || echo "test 3 failed because there is difference in expected output"
cmp -s expectedResults/get/get4.txt results/get/get4.txt && echo "test 4 completed successfully" || echo "test 4 failed because there is difference in expected output"
cmp -s expectedResults/get/get5.txt results/get/get5.txt && echo "test 5 completed successfully" || echo "test 5 failed because there is difference in expected output"
} >> results/get/summary.txt 2>&1

echo "Get tests completed"
exit 0