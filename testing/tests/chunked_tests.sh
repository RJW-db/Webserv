#!/bin/bash

# Chunked Transfer Encoding Tests
# Tests for HTTP/1.1 chunked transfer encoding compliance and bug detection

if [[ "$(pwd)" != *"Webserv"* ]]; then
    echo "Error: You must run this script from within a Webserv directory."
    exit 1
fi

cd "$(pwd | sed 's|\(.*Webserv\).*|\1|')/testing"

rm -rf results/chunked

mkdir -p results/chunked/upload

# Pre-requests to create cookies on the server
echo "Creating cookies on server..."
curl -s -o results/chunked/cookie_request.txt -i -H "Connection: close" http://localhost:15000/
sleep 0.5
# Extract session_id from the Set-Cookie header in the response
session_id=$(grep -i "Set-Cookie:" results/chunked/cookie_request.txt | grep -o 'session_id=[^;]*' | cut -d= -f2)
if [[ -z "$session_id" ]]; then
    echo "Error: session_id not found in cookie response."
    exit 1
fi
echo "Extracted session_id: $session_id"

echo "🧪 Chunked Transfer Encoding Tests"
echo "================================="

# test 1: Basic chunked POST (using curl for baseline)
echo "1. Testing basic multipart/form chunked POST request with curl"
curl -s -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Cookie: session_id=$session_id" -H "Host: server4" \
-F "file=@expectedResults/post/upload1/small.txt;filename=chunked_test1.txt" \
http://localhost:15002/upload > results/chunked/chunked1.txt &

# test 2: Another small text file via chunked multipart
echo "2. Testing second chunked multipart upload (small2.txt)"
curl -s -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Cookie: session_id=$session_id" -H "Host: server4" \
-F "file=@expectedResults/post/upload1/small2.txt;filename=chunked_test2.txt" \
http://localhost:15002/upload > results/chunked/chunked2.txt &

# test 3: Third chunked multipart upload (small3.txt)
echo "3. Testing third chunked multipart upload (small3.txt)"
curl -s -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Cookie: session_id=$session_id" -H "Host: server4" \
-F "file=@expectedResults/post/upload1/small3.txt;filename=chunked_test3.txt" \
http://localhost:15002/upload > results/chunked/chunked3.txt &

# test 4: multiple chunks
echo "4. Testing multiple chunks (small4.txt)"
{
    #<chunk-size>\r\n
    #<chunk-data>\r\n

    # HTTP headers
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: example.com\r\n"
    echo -ne "Cookie: session_id=$session_id\r\n"
    echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n\r\n"

    # sleep 0.5
    # Part 1: multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename="chunked_test4.txt"\r\nContent-Type: text/plain\r\n\r\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK1" | wc -c)    #size of chunk, HEX = 89, DEC = 137
    printf "%s\r\n" "$CHUNK1"                           #data of chunk
    sleep 0.5

    # Part 2: file content
    CHUNK2=$'This is the content of the file.\nSecond line of text.\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK2" | wc -c)
    printf "%s\r\n" "$CHUNK2"
    sleep 0.5

    # Part 3: file content
    CHUNK3=$'third line of text\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK3" | wc -c)
    printf "%s\r\n" "$CHUNK3"
    sleep 0.5

    # Part 4: ending boundary
    END_BOUNDARY=$'------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n'
    printf "%X\r\n" $(printf "%s" "$END_BOUNDARY" | wc -c)
    printf "%s\r\n" "$END_BOUNDARY"
    sleep 0.5

    # # Final zero-length chunk
    echo -ne "0\r\n\r\n"

} | nc localhost 15002 > results/chunked/chunked4.txt &

# test 5: multiple chunks
echo "5. Testing multiple chunks (small5.txt)"
{
    #<chunk-size>\r\n
    #<chunk-data>\r\n

    # HTTP headers
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: example.com\r\n"
    echo -ne "Cookie: session_id=$session_id\r\n"
    echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n\r\n"

    # sleep 0.5
    # Part 1: partial multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename='
    printf "%X\r\n" $(printf "%s" "$CHUNK1" | wc -c)
    printf "%s\r\n" "$CHUNK1"
    sleep 0.5

    # Part 2: rest of header and start of content
    CHUNK2=$'"chunked_test5.txt"\r\nContent-Type: text/plain\r\n\r\nThis is the content'
    printf "%X\r\n" $(printf "%s" "$CHUNK2" | wc -c)
    printf "%s\r\n" "$CHUNK2"
    sleep 0.5

    # Part 3: middle of file content
    CHUNK3=$' of the file.\nSecond line of'
    printf "%X\r\n" $(printf "%s" "$CHUNK3" | wc -c)
    printf "%s\r\n" "$CHUNK3"
    sleep 0.5

    # Part 4: more file content
    CHUNK4=$' text.\nthird line of text\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK4" | wc -c)
    printf "%s\r\n" "$CHUNK4"
    sleep 0.5

    # Part 5: start of ending boundary
    CHUNK5=$'------WebKitFormBoundary7MA4YWxkTrZu0gW-'
    printf "%X\r\n" $(printf "%s" "$CHUNK5" | wc -c)
    printf "%s\r\n" "$CHUNK5"
    sleep 0.5

    # Part 6: end of boundary
    CHUNK6=$'-\r\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK6" | wc -c)
    printf "%s\r\n" "$CHUNK6"
    sleep 0.5

    # Final zero-length chunk
    echo -ne "0\r\n\r\n"

} | nc localhost 15002 > results/chunked/chunked5.txt &

# test 6: multiple chunks
echo "6. Testing multiple chunks (small6.txt)"
{
    # HTTP headers
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: example.com\r\n"
    echo -ne "Cookie: session_id=$session_id\r\n"
    echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n\r\n"

    # Chunk 1: multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename="chunked_test6.txt"\r\nContent-Type: text/plain\r\n\r\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK1" | wc -c)
    printf "%s\r\n" "$CHUNK1"

    # Chunk 2: file content
    CHUNK2=$'First line of multi-chunk test.\nSecond line.\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK2" | wc -c)
    printf "%s\r\n" "$CHUNK2"

    # Chunk 3: more file content
    CHUNK3=$'Third line of multi-chunk test.\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK3" | wc -c)
    printf "%s\r\n" "$CHUNK3"

    # Ending boundary
    END_BOUNDARY=$'------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n'
    printf "%X\r\n" $(printf "%s" "$END_BOUNDARY" | wc -c)
    printf "%s\r\n" "$END_BOUNDARY"

    # Final zero-length chunk
    echo -ne "0\r\n\r\n"
} | nc localhost 15002 > results/chunked/chunked6.txt &

wait
sleep 0.1

cd ..
{

# test 1: Check results
# Extract the filename from the server response (last line of the response)
actual_filename1=$(tail -n 1 testing/results/chunked/chunked1.txt | tr -d '\r\n')
expected_response_content="./testing/expectedResults/chunked/chunked_test1.txt"


# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" testing/results/chunked/chunked1.txt && grep -q "Content-Type: text/plain" testing/results/chunked/chunked1.txt; then
    if [[ -f "$actual_filename1" ]] && cmp -s "$expected_response_content" "$actual_filename1"; then
        echo "chunked test 1 completed successfully"
    else
        echo "chunked test 1 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename1" ]] && echo "  - File $actual_filename1 does not exist"
    fi
else
    echo "chunked test 1 failed because HTTP response headers are incorrect"
fi

# test 2: Check results
# Extract the filename from the server response (last line of the response)
actual_filename2=$(tail -n 1 testing/results/chunked/chunked2.txt | tr -d '\r\n')
expected_response_content="./testing/expectedResults/chunked/chunked_test2.txt"

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" testing/results/chunked/chunked2.txt && grep -q "Content-Type: text/plain" testing/results/chunked/chunked2.txt; then
    if [[ -f "$actual_filename2" ]] && cmp -s "$expected_response_content" "$actual_filename2"; then
        echo "chunked test 2 completed successfully"
    else
        echo "chunked test 2 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename2" ]] && echo "  - File $actual_filename2 does not exist"
    fi
else
    echo "chunked test 2 failed because HTTP response headers are incorrect"
fi

# test 3: Check results
# Extract the filename from the server response (last line of the response)
actual_filename3=$(tail -n 1 testing/results/chunked/chunked3.txt | tr -d '\r\n')
expected_response_content="./testing/expectedResults/chunked/chunked_test3.txt"

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" testing/results/chunked/chunked3.txt && grep -q "Content-Type: text/plain" testing/results/chunked/chunked3.txt; then
    if [[ -f "$actual_filename3" ]] && cmp -s "$expected_response_content" "$actual_filename3"; then
        echo "chunked test 3 completed successfully"
    else
        echo "chunked test 3 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename3" ]] && echo "  - File $actual_filename3 does not exist"
    fi
else
    echo "chunked test 3 failed because HTTP response headers are incorrect"
fi

# test 4: Check results
# Extract the filename from the server response (last line of the response)
actual_filename4=$(tail -n 1 testing/results/chunked/chunked4.txt | tr -d '\r\n')
expected_response_content="./testing/expectedResults/chunked/chunked_test4.txt"


# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" testing/results/chunked/chunked4.txt && grep -q "Content-Type: text/plain" testing/results/chunked/chunked4.txt; then
    if [[ -f "$actual_filename4" ]] && cmp -s "$expected_response_content" "$actual_filename4"; then
        echo "chunked test 4 completed successfully"
    else
        echo "chunked test 4 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename4" ]] && echo "  - File $actual_filename4 does not exist"
    fi
else
    echo "chunked test 4 failed because HTTP response headers are incorrect"
fi

# test 5: Check results
# Extract the filename from the server response (last line of the response)
actual_filename5=$(tail -n 1 testing/results/chunked/chunked5.txt | tr -d '\r\n')
expected_response_content="./testing/expectedResults/chunked/chunked_test5.txt"

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" testing/results/chunked/chunked5.txt && grep -q "Content-Type: text/plain" testing/results/chunked/chunked5.txt; then
    if [[ -f "$actual_filename5" ]] && cmp -s "$expected_response_content" "$actual_filename5"; then
        echo "chunked test 5 completed successfully"
    else
        echo "chunked test 5 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename5" ]] && echo "  - File $actual_filename5 does not exist"
    fi
else
    echo "chunked test 5 failed because HTTP response headers are incorrect"
fi

# test 6: Check results
# Extract the filename from the server response (last line of the response)
actual_filename6=$(tail -n 1 testing/results/chunked/chunked6.txt | tr -d '\r\n')
expected_response_content="./testing/expectedResults/chunked/chunked_test6.txt"

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" testing/results/chunked/chunked6.txt && grep -q "Content-Type: text/plain" testing/results/chunked/chunked6.txt; then
    if [[ -f "$actual_filename6" ]] && cmp -s "$expected_response_content" "$actual_filename6"; then
        echo "chunked test 6 completed successfully"
    else
        echo "chunked test 6 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename6" ]] && echo "  - File $actual_filename6 does not exist"
    fi
else
    echo "chunked test 6 failed because HTTP response headers are incorrect"
fi



} > testing/results/chunked/summary.txt 2>&1

echo "Chunked tests completed (6 curl-based multipart uploads)."
exit 0