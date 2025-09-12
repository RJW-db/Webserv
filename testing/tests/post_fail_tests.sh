#!/bin/bash
# Failing POST request test suite (all multipart/form-data)
# filepath: testing/tests/post_fail_tests.sh

if [[ "$(pwd)" != *"Webserv"* ]]; then
    echo "Error: Run this script from somewhere inside the Webserv project directory" >&2
    exit 1
fi

cd "$(pwd | sed 's|\(.*Webserv\).*|\1|')/testing" || { echo "Could not cd to testing directory"; exit 1; }

rm -rf results/post_fail
mkdir -p results/post_fail results/post_fail/upload

echo "=== Starting POST Failure Tests (multipart/form-data) ==="
echo "Make sure the server is running (e.g. with test1.conf) before executing." 
echo "These tests attempt to trigger error handling for malformed / disallowed multipart POST requests." 
echo ""

#############################################
# All tests use multipart/form-data bodies  #
#############################################

# Helper for multipart body
make_multipart() {
  B="$1"
  NAME="$2"
  FILENAME="$3"
  CONTENT="$4"
  END="$5"
  printf -- "--$B\r\nContent-Disposition: form-data; name=\"$NAME\"; filename=\"$FILENAME\"\r\nContent-Type: text/plain\r\n\r\n$CONTENT\r\n$END"
}

# # Test 1: Missing Host header (multipart) -> expect 400
# echo "1. POST missing Host header (expect 400)"
# (
#   B="BOUNDARY1"
#   printf "POST /upload1 HTTP/1.1\r\nConnection: close\r\nContent-Length: 80\r\nContent-Type: multipart/form-data; boundary=$B\r\n\r\n"
#   make_multipart "$B" "myfile" "a.txt" "Hello World" "--$B--\r\n"
# ) | nc localhost 15000 > results/post_fail/fail1.txt &

# # # Test 2: POST to location without POST allowed (root '/') on server1 (multipart) -> expect 405 or 413
# echo "2. POST to / on server1 (non-upload, tiny body limit) (expect 405 or 413)"
# curl -i -X POST -H "Host: server1" -H "Connection: close" \
#   -F "myfile=@expectedResults/post/upload1/small.txt" \
#   http://localhost:15000/ > results/post_fail/fail2.txt &

# # Test 3: Content-Length smaller than actual multipart body -> expect 400
# echo "3. POST with Content-Length smaller than body to server2 (expect 400)"
# (
#   B="BOUNDARY3"
#   printf "POST /upload HTTP/1.1\r\nHost: server5\r\nConnection: close\r\nContent-Type: multipart/form-data; boundary=$B\r\nContent-Length: 30\r\n\r\n"
#   make_multipart "$B" "myfile" "b.txt" "1234567890" "--$B--\r\n" # Actual body > 30 bytes
# ) | nc localhost 15003 > results/post_fail/fail3.txt &


# # Test 5: Invalid Transfer-Encoding chunked (bad chunk size) (multipart) -> expect 400
# echo "5. POST invalid chunked encoding to server2 (expect 400)"
# (
#   B="BOUNDARY5"
#   printf "POST /upload HTTP/1.1\r\nHost: server2\r\nConnection: close\r\nTransfer-Encoding: chunked\r\nContent-Type: multipart/form-data; boundary=$B\r\n\r\n"
#   printf "Z\r\n"
#   make_multipart "$B" "myfile" "d.txt" "HelloWorld" "--$B--\r\n"
#   printf "0\r\n\r\n"
# ) | nc localhost 15003 > results/post_fail/fail4.txt &

# # Test 6: Malformed multipart (declared boundary but not closed) on server3 -> expect 400
# echo "6. POST malformed multipart to server3 (expect 400)"
# (
#   B="BOUNDARY6"
#   printf "POST /upload HTTP/1.1\r\nHost: server3\r\nConnection: close\r\nContent-Type: multipart/form-data; boundary=$B\r\nContent-Length: 123\r\n\r\n"
#   make_multipart "$B" "myfile" "e.txt" "Hello Multipart" "" # No --$B-- terminator
# ) | nc localhost 15003 > results/post_fail/fail5.txt &

# # Test 7: Directory traversal attempt in filename on server1 (multipart) -> expect 400 or sanitized
# echo "7. POST multipart with traversal filename to server1 (expect 400 or sanitized)"
# curl -i -X POST -H "Host: server1" -H "Connection: close" \
#   -F "myfile=@expectedResults/post/upload1/small.txt;filename=../../evil.txt" \
#   http://localhost:15003/upload > results/post_fail/fail6.txt &


# # Test 9: Missing Content-Type for multipart-style body on server3 -> expect 400
# echo "9. POST missing Content-Type (multipart-like body) to server3 (expect 400)"
# (
#   B="BOUNDARY9"
#   printf "POST /upload HTTP/1.1\r\nHost: server3\r\nConnection: close\r\nContent-Length: 100\r\n\r\n"
#   make_multipart "$B" "file" "x.txt" "Hello" "--$B--\r\n"
# ) | nc localhost 15003 > results/post_fail/fail7.txt &

# # Test 10: POST to non-existent location on server2 (multipart) -> expect 404
# echo "10. POST to non-existent location /nope on server2 (expect 404)"
# curl -i -X POST -H "Host: server2" -H "Connection: close" \
#   -F "myfile=@expectedResults/post/upload1/small.txt" \
#   http://localhost:15003/nope > results/post_fail/fail8.txt &

# Test 11: Oversized body against tiny limit (POST to / with >1 byte) on server1 (multipart) -> expect 413
echo "11. POST exceeding client_max_body_size at / on server1 (expect 413)"
curl -i -X POST -H "Host: server1" -H "Connection: close" \
  -F "myfile=@expectedResults/post/upload1/small.txt" \
  http://localhost:15003/tiny-limit > results/post_fail/fail11.txt &

# # Test 12: Invalid method spelling with multipart body on server2 -> expect 400/405
# echo "12. Invalid method 'POOST' with body to server2 (expect 400 or 405)"
# (
#   B="BOUNDARY12"
#   printf "POOST /upload2 HTTP/1.1\r\nHost: server2\r\nConnection: close\r\nContent-Type: multipart/form-data; boundary=$B\r\nContent-Length: 40\r\n\r\n"
#   make_multipart "$B" "myfile" "z.txt" "BODY" "--$B--\r\n"
# ) | nc localhost 15001 > results/post_fail/fail12.txt &

# # Test 13: Duplicate Content-Length headers on server3 (multipart) -> expect 400
# echo "13. Duplicate Content-Length headers to server3 (expect 400)"
# (
#   B="BOUNDARY13"
#   printf "POST /upload3 HTTP/1.1\r\nHost: server3\r\nConnection: close\r\nContent-Length: 40\r\nContent-Length: 40\r\nContent-Type: multipart/form-data; boundary=$B\r\n\r\n"
#   make_multipart "$B" "myfile" "dup.txt" "TEST" "--$B--\r\n"
# ) | nc localhost 15000 > results/post_fail/fail13.txt &

# # Test 14: Mixed Content-Length and Transfer-Encoding on server1 (multipart) -> expect 400
# echo "14. Both Content-Length and Transfer-Encoding set to server1 (expect 400)"
# (
#   B="BOUNDARY14"
#   printf "POST /upload1 HTTP/1.1\r\nHost: server1\r\nConnection: close\r\nContent-Length: 40\r\nTransfer-Encoding: chunked\r\nContent-Type: multipart/form-data; boundary=$B\r\n\r\n"
#   printf "0\r\n\r\n"
# ) | nc localhost 15000 > results/post_fail/fail14.txt &

# # Test 15: incomplete chunked request for (multipart) -> expect 400
# echo "15. Incomplete chunked request for /upload1 (multipart) -> expect 400"
# (
#   B="BOUNDARY15"
#   printf "POST /upload1 HTTP/1.1\r\nHost: server1\r\nConnection: close\r\nTransfer-Encoding: chunked\r\nContent-Type: multipart/form-data; boundary=$B\r\n\r\n"
#   printf "0\r\n\r\n"
# ) | nc localhost 15000 > results/post_fail/fail15.txt &

# echo "" 
# echo "Waiting for requests to complete..."
# wait
# sleep 3

# # Remove lines starting with Set-Cookie from all results files
# for file in results/post_fail/fail*.txt; do
#     sed -i '/^Set-Cookie/d' "$file"
# done

# echo "" 
# echo "=== Checking POST Failure Test Results ===" 

# EXPECTED_DIR="expectedResults/post_fail"
# if [[ -d "$EXPECTED_DIR" ]]; then
#   {
#   echo "=== POST Failure Test Results ==="
#   for i in {1..14}; do
#     if [[ -f "$EXPECTED_DIR/fail$i.txt" ]]; then
#       if cmp -s "$EXPECTED_DIR/fail$i.txt" "results/post_fail/fail$i.txt"; then
#         echo "post_fail test $i completed successfully"
#       else
#         echo "post_fail test $i failed (difference from expected output)"
#       fi
#     else
#       echo "post_fail test $i (no expected file to compare)"
#     fi
#   done
#   echo "" 
#   echo "=== Summary ==="
#   echo "POST failure tests completed. Review individual fail*.txt for server responses." 
#   } > results/post_fail/summary.txt 2>&1
# else
#   echo "No expectedResults/post_fail directory; skipping comparisons." | tee results/post_fail/summary.txt
# fi

# echo "Results saved to results/post_fail/summary.txt"

exit 0
