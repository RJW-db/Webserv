#!/bin/bash
# filepath: /home/saleunin/Desktop/Webserv/testing/tests/get_fail_tests.sh

cd /home/saleunin/Desktop/Webserv/testing

# Clean and create directories for results
rm -rf results/get_fail
mkdir -p results/get_fail

echo "=== Starting GET Failure Tests ==="
echo "Make sure server is running with get_fail_tests.conf"
echo "These tests are designed to fail and test error handling"
echo ""


# # Test 1: GET request missing HOST header (should return 400)
# echo "1. Testing GET request missing HOST header (should return 400)"
# printf "GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail1.txt &

# # Test 2: GET request to method not allowed location (should return 405)
# echo "2. Testing GET request to POST-only location (should return 405)"
# curl -i -X GET -H "Host: post_only_server" -H "Connection: close" \
#     http://localhost:18002/index.html > results/get_fail/fail2.txt &

# # Test 3: GET request with wrong path not starting with / (should return 400)
# echo "3. Testing GET request with invalid path (should return 400)"
# printf "GET index.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail3.txt &

# # Test 4: GET request with different HTTP version (should return 400)
# echo "4. Testing GET request with HTTP/1.0 (should return 400)"
# printf "GET /index.html HTTP/1.0\r\nHost: get_fail_server\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail4.txt &

# # Test 5: GET request with HTTP/2.0 (should return 400)
# echo "5. Testing GET request with HTTP/2.0 (should return 400)"
# printf "GET /index.html HTTP/2.0\r\nHost: get_fail_server\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail5.txt &


# # Test 7: GET request with incorrect Connection type (should return 400)
# echo "7. Testing GET request with invalid Connection header (should return 400)"
# printf "GET /index.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: invalid-value\r\n\r\n" | nc localhost 18000 > results/get_fail/fail7.txt &

# # # Test 8: GET request with incorrect characters that turn into % (path traversal)
# # echo "8. Testing GET request with percent-encoded path traversal (should return 400)"
# # curl -i -X GET -H "Host: get_fail_server" -H "Connection: close" \
# #     "http://localhost:18000/../../../etc/passwd" > results/get_fail/fail8.txt &

# # Test 9: GET request with malformed percent encoding (should return 400)
# echo "9. Testing GET request with malformed percent encoding (should return 400)"
# curl -i -X GET -H "Host: get_fail_server" -H "Connection: close" \
#     "http://localhost:18000/test%GG.html" > results/get_fail/fail9.txt &

# # Test 13: GET request with extremely long URI (should return 414)
# echo "13. Testing GET request with extremely long URI (should return 414)"
# LONG_PATH=$(printf 'a%.0s' {1..8000})
# curl -i -X GET -H "Host: get_fail_server" -H "Connection: close" \
#     "http://localhost:18000/$LONG_PATH" > results/get_fail/fail13.txt &

# # Test 14: GET request with null bytes in path (should return 400)
# echo "14. Testing GET request with null bytes in path (should return 400)"
# printf "GET /test\x00.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail14.txt &

# # Test 15: GET request with invalid method (should return 405)
# echo "15. Testing invalid method INVALID (should return 405)"
# printf "INVALID /index.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail15.txt &

# # Test 16: GET request with malformed request line (should return 400)
# echo "16. Testing GET request with malformed request line (should return 400)"
# printf "GET HTTP/1.1\r\nHost: get_fail_server\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail16.txt &

# # Test 17: GET request with spaces in path (should return 400)
# echo "17. Testing GET request with spaces in path (should return 400)"
# printf "GET /test file.html HTTP/1.1\r\nHost: get_fail_server\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail17.txt &


# Test 19: GET request with extremely long header value (should return 400)
echo "19. Testing GET request with extremely long header value (should return 400)"
LONG_VALUE=$(printf 'a%.0s' {1..8000})
curl -i -X GET -H "Host: get_fail_server" -H "Connection: close" \
    -H "X-Long-Header: $LONG_VALUE" \
    http://localhost:18000/index.html > results/get_fail/fail19.txt &

# # Test 20: GET request with invalid characters in header name (should return 400)
# echo "20. Testing GET request with invalid characters in header name (should return 400)"
# printf "GET /index.html HTTP/1.1\r\nHost: get_fail_server\r\nInvalid-Header\x00: value\r\nConnection: close\r\n\r\n" | nc localhost 18000 > results/get_fail/fail20.txt &

echo ""
echo "Waiting for all requests to complete..."
sleep 3

echo ""
echo "=== Checking GET Failure Test Results ==="

{
echo "=== GET Failure Test Results ==="
echo ""

# # Test 1: Missing HOST header
# echo "Test 1 (missing HOST header):"
# if [ -f results/get_fail/fail1.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail1.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail1.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 2: Method not allowed
# echo "Test 2 (method not allowed):"
# if [ -f results/get_fail/fail2.txt ]; then
#     if grep -q "405\|Method Not Allowed" results/get_fail/fail2.txt; then
#         echo "  SUCCESS: Server returned 405 Method Not Allowed"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail2.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 3: Wrong path format
# echo "Test 3 (wrong path format):"
# if [ -f results/get_fail/fail3.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail3.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail3.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 4: HTTP/1.0 version
# echo "Test 4 (HTTP/1.0 version):"
# if [ -f results/get_fail/fail4.txt ]; then
#     if grep -q "400\|Bad Request\|505\|HTTP Version Not Supported" results/get_fail/fail4.txt; then
#         echo "  SUCCESS: Server rejected HTTP/1.0"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail4.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 5: HTTP/2.0 version
# echo "Test 5 (HTTP/2.0 version):"
# if [ -f results/get_fail/fail5.txt ]; then
#     if grep -q "400\|Bad Request\|505\|HTTP Version Not Supported" results/get_fail/fail5.txt; then
#         echo "  SUCCESS: Server rejected HTTP/2.0"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail5.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 6: Accept header
# echo "Test 6 (Accept: application/json):"
# if [ -f results/get_fail/fail6.txt ]; then
#     STATUS=$(head -1 results/get_fail/fail6.txt)
#     echo "  RESULT: $STATUS (behavior depends on server implementation)"
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 7: Invalid Connection header
# echo "Test 7 (invalid Connection header):"
# if [ -f results/get_fail/fail7.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail7.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail7.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 8: Path traversal
# echo "Test 8 (path traversal):"
# if [ -f results/get_fail/fail8.txt ]; then
#     if grep -q "400\|Bad Request\|403\|Forbidden" results/get_fail/fail8.txt; then
#         echo "  SUCCESS: Server blocked path traversal"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail8.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 9: Malformed percent encoding
# echo "Test 9 (malformed percent encoding):"
# if [ -f results/get_fail/fail9.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail9.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail9.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 10: GET with body
# echo "Test 10 (GET with body):"
# if [ -f results/get_fail/fail10.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail10.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail10.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 11: Duplicate Host headers
# echo "Test 11 (duplicate Host headers):"
# if [ -f results/get_fail/fail11.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail11.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail11.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 12: Duplicate Connection headers
# echo "Test 12 (duplicate Connection headers):"
# if [ -f results/get_fail/fail12.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail12.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail12.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 13: Extremely long URI
# echo "Test 13 (extremely long URI):"
# if [ -f results/get_fail/fail13.txt ]; then
#     if grep -q "414\|URI Too Long\|400\|Bad Request" results/get_fail/fail13.txt; then
#         echo "  SUCCESS: Server rejected long URI"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail13.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 14: Null bytes in path
# echo "Test 14 (null bytes in path):"
# if [ -f results/get_fail/fail14.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail14.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail14.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 15: Invalid method
# echo "Test 15 (invalid method):"
# if [ -f results/get_fail/fail15.txt ]; then
#     if grep -q "405\|Method Not Allowed\|400\|Bad Request" results/get_fail/fail15.txt; then
#         echo "  SUCCESS: Server rejected invalid method"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail15.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 16: Malformed request line
# echo "Test 16 (malformed request line):"
# if [ -f results/get_fail/fail16.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail16.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail16.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 17: Spaces in path
# echo "Test 17 (spaces in path):"
# if [ -f results/get_fail/fail17.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail17.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail17.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 18: Non-existent host
# echo "Test 18 (non-existent host):"
# if [ -f results/get_fail/fail18.txt ]; then
#     if grep -q "404\|Not Found\|400\|Bad Request" results/get_fail/fail18.txt; then
#         echo "  SUCCESS: Server handled non-existent host"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail18.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 19: Extremely long header value
# echo "Test 19 (extremely long header value):"
# if [ -f results/get_fail/fail19.txt ]; then
#     if grep -q "400\|Bad Request\|431\|Request Header Fields Too Large" results/get_fail/fail19.txt; then
#         echo "  SUCCESS: Server rejected long header value"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail19.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

# # Test 20: Invalid characters in header name
# echo "Test 20 (invalid characters in header name):"
# if [ -f results/get_fail/fail20.txt ]; then
#     if grep -q "400\|Bad Request" results/get_fail/fail20.txt; then
#         echo "  SUCCESS: Server returned 400 Bad Request"
#     else
#         echo "  RESULT: $(head -1 results/get_fail/fail20.txt)"
#     fi
# else
#     echo "  FAILED: No response file generated"
# fi

echo ""
echo "=== Summary ==="
echo "These tests verify proper error handling for various malformed GET requests."
echo "Expected responses: 400 (Bad Request), 405 (Method Not Allowed), 414 (URI Too Long), etc."
echo "Check individual result files in results/get_fail/ for detailed responses."

} > results/get_fail/summary.txt 2>&1

echo ""
echo "GET failure tests completed"
echo "Results saved to results/get_fail/summary.txt"
cat results/get_fail/summary.txt