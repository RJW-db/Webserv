#!/bin/bash

cd /home/saleunin/Desktop/Webserv/testing 

rm -rf results/post_advanced

# Create directories for results
mkdir -p results/post_advanced/form results/post_advanced/json results/post_advanced/small_limit
mkdir -p results/post_advanced/chunked results/post_advanced/multi_upload results/post_advanced/tiny_limit
mkdir -p results/post_advanced/test_errors results/post_advanced/get_only

echo "=== Starting Advanced POST Tests ==="
echo "Make sure server is running with post_advanced_tests.conf"
echo ""
# # Test 1: Simple POST with text/plain
# echo "1. Testing simple POST with text/plain"
# curl -i -X POST -H "Content-Type: text/plain" -H "Connection: close" -H "HOST: advanced_post_server" \
#     -d "Hello, this is test 1" \
#     http://localhost:16000/form > results/post_advanced/post1.txt &
# CURL_PID=$!
# sleep 1
# kill $CURL_PID 2>/dev/null

# # Test 2: POST with empty body
# echo "2. Testing POST with empty body"
# curl -i -X POST -H "Content-Type: text/plain" -H "Connection: close" \
#     -d "" \
#     http://localhost:16000/form > results/post_advanced/post2.txt &

# # Test 3: POST with JSON data
# echo "3. Testing POST with JSON data"
# curl -i -X POST -H "Content-Type: application/json" -H "Connection: close" \
#     -d '{"test":3,"success":true}' \
#     http://localhost:16000/json > results/post_advanced/post3.txt &

# # Test 4: POST with form-urlencoded data
# echo "4. Testing POST with form-urlencoded data"
# curl -i -X POST -H "Content-Type: application/x-www-form-urlencoded" -H "Connection: close" \
#     -d "foo=bar&baz=qux" \
#     http://localhost:16000/form > results/post_advanced/post4.txt &

# # Test 5: POST with special characters
# echo "5. Testing POST with special characters"
# curl -i -X POST -H "Content-Type: application/x-www-form-urlencoded" -H "Connection: close" \
#     -d "name=Jöhn&emoji=🔥&quote='\"&symbols=!@#$%^&*()" \
#     http://localhost:16000/form > results/post_advanced/post5.txt &

# # Test 6: POST with application/x-www-form-urlencoded
# echo "6. Testing POST with form data (application/x-www-form-urlencoded)"
# curl -i -X POST -H "Content-Type: application/x-www-form-urlencoded" -H "Connection: close" \
#     -d "name=John&email=john@example.com&message=Hello World" \
#     http://localhost:16000/form > results/post_advanced/post6.txt &

# # Test 7: POST with application/json
# echo "7. Testing POST with JSON data"
# curl -i -X POST -H "Content-Type: application/json" -H "Connection: close" \
#     -d '{"name":"Alice","age":30,"active":true}' \
#     http://localhost:16000/json > results/post_advanced/post7.txt &

# # Test 8: POST with text/plain
# echo "8. Testing POST with plain text data"
# curl -i -X POST -H "Content-Type: text/plain" -H "Connection: close" \
#     -d "This is plain text content for testing purposes." \
#     http://localhost:16000/form > results/post_advanced/post8.txt &

# # Test 9: POST exceeding client_max_body_size (should return 413)
# echo "9. Testing POST exceeding client_max_body_size on /small_limit (should return 413)"
# curl -i -X POST -H "Content-Type: text/plain" -H "Connection: close" \
#     -d "This data is definitely longer than 100 bytes which is the limit set for the small_limit location in our configuration file." \
#     http://localhost:16000/small_limit > results/post_advanced/post9.txt &

# # Test 10: POST to location that only allows GET (should return 405) 
# echo "10. Testing POST to GET-only location (should return 405)"
# curl -i -X POST -H "Content-Type: text/plain" -H "Connection: close" \
#     -d "This should be rejected" \
#     http://localhost:16000/get_only > results/post_advanced/post10.txt &

# # Test 11: POST with missing Content-Type header
# echo "11. Testing POST with missing Content-Type header"
# curl -i -X POST -H "Connection: close" \
#     -d "data without content type" \
#     http://localhost:16000/form > results/post_advanced/post11.txt &

# # Test 12: POST with invalid Content-Length
# echo "12. Testing POST with invalid Content-Length header"
# curl -i -X POST -H "Content-Type: text/plain" -H "Content-Length: invalid" -H "Connection: close" \
#     -d "test data" \
#     http://localhost:16000/form > results/post_advanced/post12.txt &

# # Test 13: POST with empty body
# echo "13. Testing POST with empty body"
# curl -i -X POST -H "Content-Type: text/plain" -H "Connection: close" \
#     -d "" \
#     http://localhost:16000/form > results/post_advanced/post13.txt &

# # Test 14: POST with chunked transfer encoding
# echo "14. Testing POST with chunked transfer encoding"
# echo -e "Test data for chunked transfer encoding" | curl -i -X POST \
#     -H "Transfer-Encoding: chunked" -H "Content-Type: text/plain" -H "Connection: close" \
#     --data-binary @- \
#     http://localhost:16000/chunked > results/post_advanced/post14.txt &

# # Test 15: POST with multiple files
# echo "15. Testing POST with multiple file uploads"
# # Create test files first
# echo "First test file content" > /tmp/test_file1.txt
# echo "Second test file content" > /tmp/test_file2.txt
# curl -i -X POST -H "Connection: close" \
#     -F "file1=@/tmp/test_file1.txt" \
#     -F "file2=@/tmp/test_file2.txt" \
#     -F "description=Multiple file upload test" \
#     http://localhost:16000/multi_upload > results/post_advanced/post15.txt &

# # Test 16: POST with very long headers
# echo "16. Testing POST with very long headers"
# LONG_VALUE=$(printf 'A%.0s' {1..1000})
# curl -i -X POST -H "Content-Type: text/plain" -H "X-Custom-Header: $LONG_VALUE" -H "Connection: close" \
#     -d "test with long header" \
#     http://localhost:16000/form > results/post_advanced/post16.txt &

# # Test 17: POST with special characters in data
# echo "17. Testing POST with special characters and unicode"
# curl -i -X POST -H "Content-Type: application/x-www-form-urlencoded" -H "Connection: close" \
#     -d "name=João&emoji=🚀&special=<>&\"'test" \
#     http://localhost:16000/form > results/post_advanced/post17.txt &

# # Test 18: POST to extremely small limit (should return 413)
# echo "18. Testing POST to location with 10 byte limit (should return 413)"
# curl -i -X POST -H "Content-Type: text/plain" -H "Connection: close" \
#     -d "This is way too long" \
#     http://localhost:16001/tiny_limit > results/post_advanced/post18.txt &

# # Test 19: POST with boundary edge case in multipart
# echo "19. Testing POST with multipart boundary edge case"
# echo "Edge case content" > /tmp/edge_test.txt
# curl -i -X POST -H "Connection: close" \
#     -F "file=@/tmp/edge_test.txt;filename=test file with spaces.txt" \
#     http://localhost:16000/multi_upload > results/post_advanced/post19.txt &

# # Test 20: POST with Content-Length mismatch
# echo "20. Testing POST with Content-Length mismatch"
# # This test might be tricky to implement with curl, so we'll test with a longer content-length
# curl -i -X POST -H "Content-Type: text/plain" -H "Content-Length: 100" -H "Connection: close" \
#     -d "short" \
#     http://localhost:16001/test_errors > results/post_advanced/post20.txt &

echo ""
echo "Waiting for all requests to complete..."
sleep 0.01

echo ""
echo "=== Checking Advanced POST Test Results ==="

{
# Test 6: Form data
if [ -f results/post_advanced/post6.txt ]; then
    if grep -q "200 OK" results/post_advanced/post6.txt; then
        echo "post test 6 (form data): SUCCESS - Server accepted form data"
    else
        echo "post test 6 (form data): FAILED - Server did not return 200 OK"
    fi
else
    echo "post test 6 (form data): FAILED - No response file generated"
fi

# Test 7: JSON data
if [ -f results/post_advanced/post7.txt ]; then
    if grep -q "200 OK" results/post_advanced/post7.txt; then
        echo "post test 7 (JSON data): SUCCESS - Server accepted JSON data"
    else
        echo "post test 7 (JSON data): FAILED - Server did not return 200 OK"
    fi
else
    echo "post test 7 (JSON data): FAILED - No response file generated"
fi

# Test 8: Plain text
if [ -f results/post_advanced/post8.txt ]; then
    if grep -q "200 OK" results/post_advanced/post8.txt; then
        echo "post test 8 (plain text): SUCCESS - Server accepted plain text"
    else
        echo "post test 8 (plain text): FAILED - Server did not return 200 OK"
    fi
else
    echo "post test 8 (plain text): FAILED - No response file generated"
fi

# Test 9: Body size limit exceeded
if [ -f results/post_advanced/post9.txt ]; then
    if grep -q "413" results/post_advanced/post9.txt || grep -q "Payload Too Large" results/post_advanced/post9.txt; then
        echo "post test 9 (size limit): SUCCESS - Server correctly returned 413"
    else
        echo "post test 9 (size limit): FAILED - Server should have returned 413"
    fi
else
    echo "post test 9 (size limit): FAILED - No response file generated"
fi

# Test 10: Method not allowed
if [ -f results/post_advanced/post10.txt ]; then
    if grep -q "405" results/post_advanced/post10.txt || grep -q "Method Not Allowed" results/post_advanced/post10.txt; then
        echo "post test 10 (method not allowed): SUCCESS - Server correctly returned 405"
    else
        echo "post test 10 (method not allowed): FAILED - Server should have returned 405"
    fi
else
    echo "post test 10 (method not allowed): FAILED - No response file generated"
fi

# Test 11: Missing Content-Type
if [ -f results/post_advanced/post11.txt ]; then
    if grep -q "200 OK" results/post_advanced/post11.txt || grep -q "400" results/post_advanced/post11.txt; then
        echo "post test 11 (missing content-type): Server handled missing Content-Type"
    else
        echo "post test 11 (missing content-type): FAILED - Unexpected response"
    fi
else
    echo "post test 11 (missing content-type): FAILED - No response file generated"
fi

# Test 12: Invalid Content-Length
if [ -f results/post_advanced/post12.txt ]; then
    if grep -q "400" results/post_advanced/post12.txt || grep -q "Bad Request" results/post_advanced/post12.txt; then
        echo "post test 12 (invalid content-length): SUCCESS - Server correctly handled invalid Content-Length"
    else
        echo "post test 12 (invalid content-length): Server response: $(head -1 results/post_advanced/post12.txt)"
    fi
else
    echo "post test 12 (invalid content-length): FAILED - No response file generated"
fi

# Test 13: Empty body
if [ -f results/post_advanced/post13.txt ]; then
    if grep -q "200 OK" results/post_advanced/post13.txt; then
        echo "post test 13 (empty body): SUCCESS - Server accepted empty body"
    else
        echo "post test 13 (empty body): Server response: $(head -1 results/post_advanced/post13.txt)"
    fi
else
    echo "post test 13 (empty body): FAILED - No response file generated"
fi

# Test 14: Chunked transfer
if [ -f results/post_advanced/post14.txt ]; then
    if grep -q "200 OK" results/post_advanced/post14.txt; then
        echo "post test 14 (chunked transfer): SUCCESS - Server handled chunked encoding"
    else
        echo "post test 14 (chunked transfer): Server response: $(head -1 results/post_advanced/post14.txt)"
    fi
else
    echo "post test 14 (chunked transfer): FAILED - No response file generated"
fi

# Test 15: Multiple files
if [ -f results/post_advanced/post15.txt ]; then
    if grep -q "200 OK" results/post_advanced/post15.txt; then
        echo "post test 15 (multiple files): SUCCESS - Server accepted multiple files"
    else
        echo "post test 15 (multiple files): Server response: $(head -1 results/post_advanced/post15.txt)"
    fi
else
    echo "post test 15 (multiple files): FAILED - No response file generated"
fi

# Test 16: Long headers
if [ -f results/post_advanced/post16.txt ]; then
    if grep -q "200 OK" results/post_advanced/post16.txt || grep -q "431" results/post_advanced/post16.txt; then
        echo "post test 16 (long headers): Server handled long headers"
    else
        echo "post test 16 (long headers): Server response: $(head -1 results/post_advanced/post16.txt)"
    fi
else
    echo "post test 16 (long headers): FAILED - No response file generated"
fi

# Test 17: Special characters
if [ -f results/post_advanced/post17.txt ]; then
    if grep -q "200 OK" results/post_advanced/post17.txt; then
        echo "post test 17 (special chars): SUCCESS - Server handled special characters"
    else
        echo "post test 17 (special chars): Server response: $(head -1 results/post_advanced/post17.txt)"
    fi
else
    echo "post test 17 (special chars): FAILED - No response file generated"
fi

# Test 18: Extremely small limit
if [ -f results/post_advanced/post18.txt ]; then
    if grep -q "413" results/post_advanced/post18.txt || grep -q "Payload Too Large" results/post_advanced/post18.txt; then
        echo "post test 18 (tiny limit): SUCCESS - Server correctly returned 413"
    else
        echo "post test 18 (tiny limit): Server response: $(head -1 results/post_advanced/post18.txt)"
    fi
else
    echo "post test 18 (tiny limit): FAILED - No response file generated"
fi

# Test 19: Multipart boundary edge case
if [ -f results/post_advanced/post19.txt ]; then
    if grep -q "200 OK" results/post_advanced/post19.txt; then
        echo "post test 19 (boundary edge): SUCCESS - Server handled multipart boundary edge case"
    else
        echo "post test 19 (boundary edge): Server response: $(head -1 results/post_advanced/post19.txt)"
    fi
else
    echo "post test 19 (boundary edge): FAILED - No response file generated"
fi

# Test 20: Content-Length mismatch
if [ -f results/post_advanced/post20.txt ]; then
    if grep -q "400" results/post_advanced/post20.txt || grep -q "200 OK" results/post_advanced/post20.txt; then
        echo "post test 20 (content-length mismatch): Server handled Content-Length mismatch"
    else
        echo "post test 20 (content-length mismatch): Server response: $(head -1 results/post_advanced/post20.txt)"
    fi
else
    echo "post test 20 (content-length mismatch): FAILED - No response file generated"
fi

echo ""
echo "=== Advanced POST Tests Summary ==="
echo "Check results/post_advanced/ directory for detailed responses"
echo "Review each test result above for compliance with HTTP standards"

} > results/post_advanced/summary.txt 2>&1

# Clean up temporary files
rm -f /tmp/test_file1.txt /tmp/test_file2.txt /tmp/edge_test.txt

echo "Advanced POST tests completed"
echo "Results saved to results/post_advanced/summary.txt"
