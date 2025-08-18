#!/bin/bash

# Chunked Transfer Encoding Tests
# Tests for HTTP/1.1 chunked transfer encoding compliance and bug detection

if [[ "$(pwd)" != *"Webserv"* ]]; then
    echo "Error: You must run this script from within a Webserv directory."
    exit 1
fi

cd "$(pwd | sed 's|\(.*Webserv\).*|\1|')/testing"

mkdir -p results/chunked



echo "🧪 Chunked Transfer Encoding Tests"
echo "================================="

# # test 1: Basic chunked POST (using curl for baseline)
# echo "1. Testing basic multipart/form chunked POST request with curl"
# curl -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Host: server1" \
# -F "file=@expectedResults/post/upload1/small.txt;filename=chunked_test.txt" \
# # -F "description=Test chunked multipart upload" \
# http://localhost:15002/upload3 > results/chunked/chunked1.txt &

# test 3: Malformed chunk size (should be handled gracefully)
echo "3. Testing malformed chunk size (should return 400 Bad Request)"
{
    echo -ne "POST /upload3 HTTP/1.1\r\n"
    echo -ne "Host: server1\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n"
    echo -ne "Connection: close\r\n\r\n"
    
    # Invalid hex chunk size
    echo -ne "GG\r\nInvalid\r\n0\r\n\r\n"
} | nc localhost 15002 > results/chunked/chunked3.txt 2>&1 &

# # test 4: Zero chunk with trailer headers (should expose end-detection bug)
# echo "4. Testing zero chunk with trailer headers (exposes end-detection bug)"
# {
#     echo -ne "POST /upload3 HTTP/1.1\r\n"
#     echo -ne "Host: server1\r\n"
#     echo -ne "Transfer-Encoding: chunked\r\n"
#     echo -ne "Connection: close\r\n\r\n"
    
#     # Simple data chunk
#     echo -ne "5\r\nHello\r\n"
    
#     # Zero chunk with trailer
#     echo -ne "0\r\n"
#     echo -ne "X-Custom-Trailer: test-value\r\n"
#     echo -ne "Content-MD5: abcd1234\r\n"
#     echo -ne "\r\n"
# } | nc localhost 15000 > results/chunked/chunked4.txt 2>&1 &

# # test 5: Multiple small chunks (stress test chunk parsing)
# echo "5. Testing multiple small chunks (stress test)"
# {
#     echo -ne "POST /upload3 HTTP/1.1\r\n"
#     echo -ne "Host: server1\r\n"
#     echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
#     echo -ne "Transfer-Encoding: chunked\r\n"
#     echo -ne "Connection: close\r\n\r\n"
    
#     # Many small chunks
#     for i in {1..10}; do
#         CHUNK="$i"
#         printf "%X\r\n" $(printf "%s" "$CHUNK" | wc -c)
#         printf "%s\r\n" "$CHUNK"
#     done
    
#     # Final chunk
#     echo -ne "0\r\n\r\n"
# } | nc localhost 15000 > results/chunked/chunked5.txt 2>&1 &

# # test 6: Chunk with missing CRLF (should be detected as malformed)
# echo "6. Testing chunk with missing CRLF (should return 400)"
# {
#     echo -ne "POST /upload3 HTTP/1.1\r\n"
#     echo -ne "Host: server1\r\n"
#     echo -ne "Transfer-Encoding: chunked\r\n"
#     echo -ne "Connection: close\r\n\r\n"
    
#     # Chunk missing CRLF after data
#     echo -ne "5\r\nHello"  # Missing \r\n after data
#     echo -ne "0\r\n\r\n"
# } | nc localhost 15000 > results/chunked/chunked6.txt 2>&1 &

# # test 7: Simple chunk extensions (minimal test case)
# echo "7. Testing simple chunk extensions"
# {
#     echo -ne "POST /upload3 HTTP/1.1\r\n"
#     echo -ne "Host: server1\r\n"
#     echo -ne "Transfer-Encoding: chunked\r\n"
#     echo -ne "Connection: close\r\n\r\n"
    
#     # Simple chunk with extension
#     echo -ne "5;ext=value\r\nHello\r\n"
#     echo -ne "0\r\n\r\n"
# } | nc localhost 15000 > results/chunked/chunked7.txt 2>&1 &

# # test 8: Chunk size edge cases
# echo "8. Testing chunk size edge cases"
# {
#     echo -ne "POST /upload3 HTTP/1.1\r\n"
#     echo -ne "Host: server1\r\n"
#     echo -ne "Transfer-Encoding: chunked\r\n"
#     echo -ne "Connection: close\r\n\r\n"
    
#     # Zero-length chunk in middle (should be ignored)
#     echo -ne "3\r\nABC\r\n"
#     echo -ne "0\r\n\r\n"  # Final zero chunk
#     echo -ne "0\r\n\r\n"  # Extra zero chunk (malformed)
# } | nc localhost 15000 > results/chunked/chunked8.txt 2>&1 &

# sleep 3

# {
# echo "🔍 Analyzing Chunked Encoding Test Results"
# echo "=========================================="

# # test 1: Check basic chunked transfer
# actual_filename1=$(tail -n 1 results/chunked/chunked1.txt | tr -d '\r\n')
# if grep -q "HTTP/1.1 201 Created" results/chunked/chunked1.txt && grep -q "Content-Type: text/plain" results/chunked/chunked1.txt; then
#     if [[ -f "$actual_filename1" ]] && cmp -s expectedResults/post/upload1/small.txt "$actual_filename1"; then
#         echo "✅ Test 1 PASSED: Basic chunked transfer works"
#     else
#         echo "❌ Test 1 FAILED: File content doesn't match"
#         [[ ! -f "$actual_filename1" ]] && echo "  - File $actual_filename1 does not exist"
#     fi
# else
#     echo "❌ Test 1 FAILED: HTTP response headers are incorrect"
# fi

# # test 2: Check chunked with extensions (should fail with current implementation)
# echo ""
# echo "🐛 Bug Detection Tests:"
# echo "======================"
# if grep -q "HTTP/1.1 400" results/chunked/chunked2.txt && grep -q "non-hex" results/chunked/chunked2.txt; then
#     echo "❌ Test 2 FAILED AS EXPECTED - server rejects valid chunk extensions (BUG CONFIRMED)"
#     echo "  🐛 BUG: validateChunkSizeLine() rejects valid chunk extensions like 'A;charset=utf-8'"
#     echo "  📍 Location: HandleTransferChunks.cpp - validateChunkSizeLine() function"
#     echo "  🔧 Fix: Parse only hex digits before semicolon, ignore extensions"
# elif grep -q "HTTP/1.1 201" results/chunked/chunked2.txt; then
#     echo "✅ Test 2 PASSED: Server correctly accepts chunk extensions"
# else
#     echo "⚠️  Test 2 UNCLEAR: Unexpected response - check results/chunked/chunked2.txt"
# fi

# # test 3: Check malformed chunk size handling
# if grep -q "HTTP/1.1 400" results/chunked/chunked3.txt; then
#     echo "✅ Test 3 PASSED: Server correctly rejects malformed chunk size"
# elif grep -q "HTTP/1.1 500" results/chunked/chunked3.txt; then
#     echo "❌ Test 3 FAILED: Server crashed on malformed chunk (should return 400)"
# else
#     echo "❌ Test 3 FAILED: Server should return 400 for malformed chunk size"
# fi

# # test 4: Check trailer handling (likely to expose end-detection bug)
# if grep -q "HTTP/1.1 201" results/chunked/chunked4.txt; then
#     echo "✅ Test 4 PASSED: Server handles trailers correctly"
# elif grep -q "HTTP/1.1 400" results/chunked/chunked4.txt; then
#     echo "❌ Test 4 FAILED AS EXPECTED - server rejects trailers (BUG CONFIRMED)"
#     echo "  🐛 BUG: decodeChunk() end-detection logic is incorrect"
#     echo "  📍 Location: HandleTransferChunks.cpp - decodeChunk() function"
#     echo "  🔧 Fix: Properly handle trailer headers after final chunk"
# elif [ ! -s results/chunked/chunked4.txt ]; then
#     echo "❌ Test 4 FAILED: Server hangs on trailer headers (BUG CONFIRMED)"
#     echo "  🐛 BUG: End-detection logic causes infinite loop"
# else
#     echo "⚠️  Test 4 UNCLEAR: Unexpected response to trailer headers"
# fi

# # test 5: Check multiple small chunks
# if grep -q "HTTP/1.1 201\|HTTP/1.1 200" results/chunked/chunked5.txt; then
#     echo "✅ Test 5 PASSED: Server handles multiple small chunks"
# else
#     echo "❌ Test 5 FAILED: Server failed on multiple small chunks"
# fi

# # test 6: Check missing CRLF detection
# if grep -q "HTTP/1.1 400" results/chunked/chunked6.txt; then
#     echo "✅ Test 6 PASSED: Server correctly detects missing CRLF"
# elif [ ! -s results/chunked/chunked6.txt ]; then
#     echo "❌ Test 6 FAILED AS EXPECTED - server hangs on malformed chunk (BUG CONFIRMED)"
#     echo "  🐛 BUG: Missing CRLF validation can cause server to hang"
#     echo "  📍 Location: HandleTransferChunks.cpp - chunk data validation"
#     echo "  🔧 Fix: Add proper CRLF validation after chunk data"
# elif grep -q "timeout\|connection" results/chunked/chunked6.txt; then
#     echo "❌ Test 6 FAILED: Server timeout on malformed chunk"
# else
#     echo "⚠️  Test 6 UNCLEAR: Unexpected response to malformed chunk"
# fi

# # test 7: Check simple chunk extensions
# if grep -q "HTTP/1.1 400" results/chunked/chunked7.txt && grep -q "non-hex" results/chunked/chunked7.txt; then
#     echo "❌ Test 7 FAILED AS EXPECTED - server rejects simple chunk extensions (BUG CONFIRMED)"
#     echo "  🐛 Same bug as Test 2 - validateChunkSizeLine() issue"
# elif grep -q "HTTP/1.1 201\|HTTP/1.1 200" results/chunked/chunked7.txt; then
#     echo "✅ Test 7 PASSED: Server accepts simple chunk extensions"
# else
#     echo "⚠️  Test 7 UNCLEAR: Check results/chunked/chunked7.txt"
# fi

# # test 8: Check chunk size edge cases
# if grep -q "HTTP/1.1 201\|HTTP/1.1 200" results/chunked/chunked8.txt; then
#     echo "✅ Test 8 PASSED: Server handles chunk edge cases"
# elif grep -q "HTTP/1.1 400" results/chunked/chunked8.txt; then
#     echo "✅ Test 8 PASSED: Server correctly rejects malformed chunk sequence"
# else
#     echo "❌ Test 8 FAILED: Unexpected response to chunk edge cases"
# fi

# echo ""
# echo "📊 Summary of Chunked Encoding Issues:"
# echo "====================================="
# echo "Critical bugs found in HandleTransferChunks.cpp:"
# echo ""
# echo "1. 🐛 validateChunkSizeLine() Bug (Tests 2, 7)"
# echo "   - Rejects valid chunk extensions like '5;ext=value'"
# echo "   - Uses isxdigit() on entire line instead of just hex part"
# echo "   - Fix: Parse hex digits before semicolon only"
# echo ""
# echo "2. 🐛 decodeChunk() End-Detection Bug (Test 4)"
# echo "   - Incorrect logic for detecting end of chunks"
# echo "   - May not handle trailer headers properly"
# echo "   - Fix: Improve final chunk termination detection"
# echo ""
# echo "3. 🐛 Missing CRLF Validation (Test 6)"
# echo "   - Can cause server to hang on malformed chunks"
# echo "   - No validation that chunk data ends with \\r\\n"
# echo "   - Fix: Add proper CRLF validation after chunk data"
# echo ""
# echo "🎯 Priority: Fix these bugs for full HTTP/1.1 chunked encoding compliance"
# echo "📁 Test results saved in: results/chunked/"

# } > results/chunked/summary.txt 2>&1

echo "Chunked encoding tests completed"
echo "Check results/chunked/summary.txt for detailed analysis"
exit 0
