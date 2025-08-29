#!/bin/bash
# filepath: /home/saleunin/Desktop/Webserv/testing/tests/get_fail_tests.sh

if [[ "$(pwd)" != *"Webserv"* ]]; then
    echo "Error: You must run this script from within a Webserv directory."
    exit 1
fi

cd "$(pwd | sed 's|\(.*Webserv\).*|\1|')/testing"

# Clean and create directories for results
rm -rf results/get_fail
mkdir -p results/get_fail

echo "=== Starting GET Failure Tests ==="
echo "Make sure server is running with get_fail_tests.conf"
echo "These tests are designed to fail and test error handling"
echo ""

# Test 1: GET request missing HOST header (should return 400)
echo "1. Testing GET request missing HOST header (should return 400)"
printf "GET /index.html HTTP/1.1\r\nConnection: keep-alive\r\n\r\n" | nc localhost 15003 > results/get_fail/fail1.txt &

# Test 2: GET request to method not allowed location (should return 405)
echo "2. Testing GET request to POST-only location (should return 405)"
curl -i -X GET -H "Host: post_only_server" -H "Connection: keep-alive" \
    http://localhost:15003/post_only/index.html > results/get_fail/fail2.txt &

# Test 3: GET request with wrong path not starting with / (should return 400)
echo "3. Testing GET request with invalid path (should return 400)"
printf "GET index.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: keep-alive\r\n\r\n" | nc localhost 15003 > results/get_fail/fail3.txt &

# Test 4: GET request with different HTTP version (should return 400)
echo "4. Testing GET request with HTTP/1.0 (should return 400)"
printf "GET /index.html HTTP/1.0\r\nHost: get_fail_server\r\nConnection: keep-alive\r\n\r\n" | nc localhost 15003 > results/get_fail/fail4.txt &

# Test 5: GET request with HTTP/2.0 (should return 400)
echo "5. Testing GET request with HTTP/2.0 (should return 400)"
printf "GET /index.html HTTP/2.0\r\nHost: get_fail_server\r\nConnection: keep-alive\r\n\r\n" | nc localhost 15003 > results/get_fail/fail5.txt &

# Test 6: GET request with incorrect Connection type (should return 400)
echo "6. Testing GET request with invalid Connection header (should return 400)"
printf "GET /index.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: invalid-value\r\n\r\n" | nc localhost 15003 > results/get_fail/fail6.txt &

# Test 7: GET request with incorrect characters that turn into % (path traversal)
echo "7. Testing GET request with percent-encoded path traversal (should return 400)"
curl -i -X GET -H "Host: get_fail_server" -H "Connection: keep-alive" \
"http://localhost:15003/../Makefile" > results/get_fail/fail7.txt &

# Test 8: GET request with malformed percent encoding (should return 400)
echo "8. Testing GET request with malformed percent encoding (should return 400)"
curl -i -X GET -H "Host: get_fail_server" -H "Connection: keep-alive" \
    "http://localhost:15003/test%GG.html" > results/get_fail/fail8.txt &

# Test 9: GET request with extremely long URI (should return 414)
echo "9. Testing GET request with extremely long URI (should return 414)"
LONG_PATH=$(printf 'a%.0s' {1..8000})
curl -i -X GET -H "Host: get_fail_server" -H "Connection: keep-alive" \
    "http://localhost:15003/$LONG_PATH" > results/get_fail/fail9.txt &

# Test 10: GET request with null bytes in path (should return 400)
echo "10. Testing GET request with null bytes in path (should return 400)"
printf "GET /test\x00.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: keep-alive\r\n\r\n" | nc localhost 15003 > results/get_fail/fail10.txt &

# Test 11: GET request with invalid method (should return 405)
echo "11. Testing invalid method INVALID (should return 405)"
printf "INVALID /index.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: keep-alive\r\n\r\n" | nc localhost 15003   > results/get_fail/fail11.txt &

# Test 12: GET request with malformed request line (should return 400)
echo "12. Testing GET request with malformed request line (should return 400)"
printf "GET HTTP/1.1\r\nHost: get_fail_server\r\nConnection: keep-alive\r\n\r\n" | nc localhost 15003 > results/get_fail/fail12.txt &

# Test 13: GET request with spaces in path (should return 400)
echo "13. Testing GET request with spaces in path (should return 400)"
printf "GET /test file.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: keep-alive\r\n\r\n" | nc localhost 15003 > results/get_fail/fail13.txt &

# Test 14: GET request with extremely long header value (should return 400)
echo "14. Testing GET request with extremely long header value (should return 400)"
LONG_VALUE=$(printf 'a%.0s' {1..8000})
curl -i -X GET -H "Host: get_fail_server" -H "Connection: keep-alive" \
    -H "X-Long-Header: $LONG_VALUE" \
    http://localhost:15003/index.html > results/get_fail/fail14.txt &

# Wait for all background requests
echo ""
echo "Waiting for all requests to complete..."
sleep 3

echo ""
echo "=== Checking GET Failure Test Results ==="

{
echo "=== GET Failure Test Results ==="
echo ""
# Comparisons with expected results
cmp -s expectedResults/get_fail/fail1.txt results/get_fail/fail1.txt && echo "get_fail test 1 completed successfully" || echo "get_fail test 1 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail2.txt results/get_fail/fail2.txt && echo "get_fail test 2 completed successfully" || echo "get_fail test 2 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail3.txt results/get_fail/fail3.txt && echo "get_fail test 3 completed successfully" || echo "get_fail test 3 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail4.txt results/get_fail/fail4.txt && echo "get_fail test 4 completed successfully" || echo "get_fail test 4 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail5.txt results/get_fail/fail5.txt && echo "get_fail test 5 completed successfully" || echo "get_fail test 5 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail6.txt results/get_fail/fail6.txt && echo "get_fail test 6 completed successfully" || echo "get_fail test 6 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail7.txt results/get_fail/fail7.txt && echo "get_fail test 7 completed successfully" || echo "get_fail test 7 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail8.txt results/get_fail/fail8.txt && echo "get_fail test 8 completed successfully" || echo "get_fail test 8 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail9.txt results/get_fail/fail9.txt && echo "get_fail test 9 completed successfully" || echo "get_fail test 9 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail10.txt results/get_fail/fail10.txt && echo "get_fail test 10 completed successfully" || echo "get_fail test 10 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail11.txt results/get_fail/fail11.txt && echo "get_fail test 11 completed successfully" || echo "get_fail test 11 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail12.txt results/get_fail/fail12.txt && echo "get_fail test 12 completed successfully" || echo "get_fail test 12 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail13.txt results/get_fail/fail13.txt && echo "get_fail test 13 completed successfully" || echo "get_fail test 13 failed because there is difference in expected output"
cmp -s expectedResults/get_fail/fail14.txt results/get_fail/fail14.txt && echo "get_fail test 14 completed successfully" || echo "get_fail test 14 failed because there is difference in expected output"

# echo ""
# echo "=== Summary ==="
# echo "These tests verify proper error handling for various malformed GET requests."
# echo "Expected responses: 400 (Bad Request), 405 (Method Not Allowed), 414 (URI Too Long), etc."
# echo "Check individual result files in results/get_fail/ for detailed responses."

} > results/get_fail/summary.txt 2>&1

echo ""
echo "GET failure tests completed"
echo "Results saved to results/get_fail/summary.txt"
# cat results/get_fail/summary.txt