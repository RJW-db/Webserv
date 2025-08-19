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
# (change filename and source file to small2.txt)
echo "2. Testing second chunked multipart upload (small2.txt)"
curl -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Host: server4" \
-F "file=@expectedResults/post/upload1/small2.txt;filename=chunked_test2.txt" \
http://localhost:15002/upload > results/chunked/chunked2.txt &

# test 3: Third chunked multipart upload (small3.txt)
echo "3. Testing third chunked multipart upload (small3.txt)"
curl -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Host: server4" \
-F "file=@expectedResults/post/upload1/small3.txt;filename=chunked_test3.txt" \
http://localhost:15002/upload > results/chunked/chunked3.txt &

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

