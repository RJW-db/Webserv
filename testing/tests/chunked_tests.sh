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



echo "🧪 Chunked Transfer Encoding Tests"
echo "================================="

# test 1: Basic chunked POST (using curl for baseline)
echo "1. Testing basic multipart/form chunked POST request with curl"
curl -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Host: server4" \
-F "file=@expectedResults/post/upload1/small.txt;filename=chunked_test.txt" \
http://localhost:15002/upload > results/chunked/chunked1.txt &

# test 2: Another small text file via chunked multipart
echo "2. Testing second chunked multipart upload (small2.txt)"
curl -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Host: server4" \
-F "file=@expectedResults/post/upload1/small2.txt;filename=chunked_test2.txt" \
http://localhost:15002/upload > results/chunked/chunked2.txt &

# test 3: Third chunked multipart upload (small3.txt)
echo "3. Testing third chunked multipart upload (small3.txt)"
curl -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Host: server4" \
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
    echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n\r\n"

    # sleep 1
    # Part 1: multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename="chunk_test1.txt"\r\nContent-Type: text/plain\r\n\r\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK1" | wc -c)    #size of chunk, HEX = 89, DEC = 137
    printf "%s\r\n" "$CHUNK1"                           #data of chunk
    sleep 1

    # Part 2: file content
    CHUNK2=$'This is the content of the file.\nSecond line of text.\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK2" | wc -c)
    printf "%s\r\n" "$CHUNK2"
    sleep 1

    # Part 3: file content
    CHUNK3=$'third line of text\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK3" | wc -c)
    printf "%s\r\n" "$CHUNK3"
    sleep 1

    # Part 4: ending boundary
    END_BOUNDARY=$'------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n'
    printf "%X\r\n" $(printf "%s" "$END_BOUNDARY" | wc -c)
    printf "%s\r\n" "$END_BOUNDARY"
    sleep 1

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
    echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n\r\n"

    # sleep 1
    # Part 1: partial multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename='
    printf "%X\r\n" $(printf "%s" "$CHUNK1" | wc -c)
    printf "%s\r\n" "$CHUNK1"
    sleep 1

    # Part 2: rest of header and start of content
    CHUNK2=$'"chunk_test2.txt"\r\nContent-Type: text/plain\r\n\r\nThis is the content'
    printf "%X\r\n" $(printf "%s" "$CHUNK2" | wc -c)
    printf "%s\r\n" "$CHUNK2"
    sleep 1

    # Part 3: middle of file content
    CHUNK3=$' of the file.\nSecond line of'
    printf "%X\r\n" $(printf "%s" "$CHUNK3" | wc -c)
    printf "%s\r\n" "$CHUNK3"
    sleep 1

    # Part 4: more file content
    CHUNK4=$' text.\nthird line of text\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK4" | wc -c)
    printf "%s\r\n" "$CHUNK4"
    sleep 1

    # Part 5: start of ending boundary
    CHUNK5=$'------WebKitFormBoundary7MA4YWxkTrZu0gW-'
    printf "%X\r\n" $(printf "%s" "$CHUNK5" | wc -c)
    printf "%s\r\n" "$CHUNK5"
    sleep 1

    # Part 6: end of boundary
    CHUNK6=$'-\r\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK6" | wc -c)
    printf "%s\r\n" "$CHUNK6"
    sleep 1

    # Final zero-length chunk
    echo -ne "0\r\n\r\n"

} | nc localhost 15002 > results/chunked/chunked5.txt &

# test 6: multiple chunks
echo "6. Testing multiple chunks (small6.txt)"
{
    # HTTP headers
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: example.com\r\n"
    echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n\r\n"

    # Chunk 1: multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename="chunk_test4.txt"\r\nContent-Type: text/plain\r\n\r\n'
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

sleep 2
cd ..
{
# Function to validate a single chunked test result
check_chunked_test() {
    local num="$1" expected_file="$2" result_file="$3"
    local actual_filename
    # last non-empty line assumed to be saved file path
    actual_filename=$(grep -v '^$' "$result_file" | tail -n 1 | tr -d '\r\n')

    if grep -q "HTTP/1.1 201 Created" "$result_file"; then
        if [[ -f "$actual_filename" ]]; then
            if cmp -s "$expected_file" "$actual_filename"; then
                echo "chunked test $num completed successfully"
            else
                echo "chunked test $num failed: file content mismatch"
            fi
        else
            echo "chunked test $num failed: uploaded file not found ($actual_filename)"
        fi
    else
        echo "chunked test $num failed: output not as expected"
    fi
}

check_chunked_test 1 testing/expectedResults/post/upload1/small.txt  testing/results/chunked/chunked1.txt
check_chunked_test 2 testing/expectedResults/post/upload1/small2.txt testing/results/chunked/chunked2.txt
check_chunked_test 3 testing/expectedResults/post/upload1/small3.txt testing/results/chunked/chunked3.txt
} > testing/results/chunked/summary.txt 2>&1

# cat testing/results/chunked/summary.txt

echo "Chunked tests completed (3 curl-based multipart uploads)."

