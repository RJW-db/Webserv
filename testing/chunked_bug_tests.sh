#!/bin/bash

# Chunked Encoding Bug Test Suite
# This script specifically tests for chunked transfer encoding bugs in the webserv implementation

echo "🧪 Chunked Encoding Bug Test Suite"
echo "=================================="

cd /home/saleunin/Desktop/Webserv/testing
mkdir -p results/chunked_bugs

# Test 1: Chunk Extensions (Bug in validateChunkSizeLine)
echo "Test 1: Chunk with valid extensions (should work but likely fails)"
{
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: localhost:8080\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n"
    echo -ne "Connection: close\r\n\r\n"
    
    # Valid chunk with extension per RFC 7230
    echo -ne "A;charset=utf-8\r\nHello Test\r\n"
    echo -ne "5;name=end\r\nWorld\r\n"
    echo -ne "0\r\n\r\n"
} | timeout 5 nc localhost 8080 > results/chunked_bugs/test1_extensions.txt 2>&1

# Test 2: End-of-chunks detection bug
echo "Test 2: Zero chunk end detection (exposes decodeChunk bug)"
{
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: localhost:8080\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n"
    echo -ne "Connection: close\r\n\r\n"
    
    # This should trigger the end-detection bug
    echo -ne "5\r\nHello\r\n"
    echo -ne "0\r\n\r\n"  # Final chunk - the bug is in checking this
} | timeout 5 nc localhost 8080 > results/chunked_bugs/test2_end_detection.txt 2>&1

# Test 3: Trailer headers (not supported, should be gracefully handled)
echo "Test 3: Trailer headers after final chunk"
{
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: localhost:8080\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n"
    echo -ne "Connection: close\r\n\r\n"
    
    echo -ne "D\r\nHello Trailer\r\n"
    echo -ne "0\r\n"
    echo -ne "X-Checksum: abc123\r\n"  # Trailer header
    echo -ne "\r\n"
} | timeout 5 nc localhost 8080 > results/chunked_bugs/test3_trailers.txt 2>&1

# Test 4: Malformed chunk - missing CRLF
echo "Test 4: Missing CRLF after chunk data (can cause infinite loop)"
{
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: localhost:8080\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n"
    echo -ne "Connection: close\r\n\r\n"
    
    echo -ne "5\r\nHello"  # Missing \r\n after data
    sleep 1
    echo -ne "0\r\n\r\n"
} | timeout 5 nc localhost 8080 > results/chunked_bugs/test4_missing_crlf.txt 2>&1

# Test 5: Invalid hex characters in chunk size
echo "Test 5: Invalid hex characters in chunk size"
{
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: localhost:8080\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n"
    echo -ne "Connection: close\r\n\r\n"
    
    echo -ne "GG\r\nInvalid\r\n"  # GG is not valid hex
    echo -ne "0\r\n\r\n"
} | timeout 5 nc localhost 8080 > results/chunked_bugs/test5_invalid_hex.txt 2>&1

# Test 6: Chunk size larger than declared data
echo "Test 6: Chunk size mismatch (size > actual data)"
{
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: localhost:8080\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n"
    echo -ne "Connection: close\r\n\r\n"
    
    echo -ne "10\r\nShort\r\n"  # Declares 16 bytes but only sends 5
    echo -ne "0\r\n\r\n"
} | timeout 5 nc localhost 8080 > results/chunked_bugs/test6_size_mismatch.txt 2>&1

echo ""
echo "🔍 Analyzing Results..."
echo "======================"

# Analyze Test 1: Chunk Extensions
if grep -q "400.*non-hex" results/chunked_bugs/test1_extensions.txt; then
    echo "❌ Test 1 FAILED (BUG CONFIRMED): Server rejects valid chunk extensions"
    echo "   Issue: validateChunkSizeLine() uses isxdigit() on entire line including extensions"
    echo "   Fix: Parse only hex digits before semicolon in chunk size line"
elif grep -q "HTTP/1.1 [25][0-9][0-9]" results/chunked_bugs/test1_extensions.txt; then
    echo "✅ Test 1 PASSED: Server correctly handles chunk extensions"
else
    echo "⚠️  Test 1 UNCLEAR: Check results/chunked_bugs/test1_extensions.txt"
fi

# Analyze Test 2: End Detection
if grep -q "HTTP/1.1 [25][0-9][0-9]" results/chunked_bugs/test2_end_detection.txt; then
    echo "✅ Test 2 PASSED: End-of-chunks detected correctly"
elif [ ! -s results/chunked_bugs/test2_end_detection.txt ] || grep -q "timeout" results/chunked_bugs/test2_end_detection.txt; then
    echo "❌ Test 2 FAILED (BUG CONFIRMED): Server hangs on end-of-chunks"
    echo "   Issue: decodeChunk() end-detection logic is incorrect"
    echo "   Fix: Properly detect final \\r\\n\\r\\n after zero chunk"
else
    echo "⚠️  Test 2 UNCLEAR: Check results/chunked_bugs/test2_end_detection.txt"
fi

# Analyze Test 3: Trailers
if grep -q "HTTP/1.1 [45][0-9][0-9]" results/chunked_bugs/test3_trailers.txt; then
    echo "⚠️  Test 3 EXPECTED: Server doesn't support trailers (optional feature)"
elif grep -q "HTTP/1.1 [25][0-9][0-9]" results/chunked_bugs/test3_trailers.txt; then
    echo "✅ Test 3 PASSED: Server handles trailers gracefully"
else
    echo "❌ Test 3 FAILED: Server crashed or hung on trailers"
fi

# Analyze Test 4: Missing CRLF
if grep -q "HTTP/1.1 400" results/chunked_bugs/test4_missing_crlf.txt; then
    echo "✅ Test 4 PASSED: Server correctly rejects malformed chunk"
elif [ ! -s results/chunked_bugs/test4_missing_crlf.txt ]; then
    echo "❌ Test 4 FAILED (BUG CONFIRMED): Server hangs on missing CRLF"
    echo "   Issue: Missing validation for CRLF after chunk data"
    echo "   Fix: Add proper CRLF validation in chunk parsing"
else
    echo "⚠️  Test 4 UNCLEAR: Check results/chunked_bugs/test4_missing_crlf.txt"
fi

# Analyze Test 5: Invalid Hex
if grep -q "HTTP/1.1 400" results/chunked_bugs/test5_invalid_hex.txt; then
    echo "✅ Test 5 PASSED: Server correctly rejects invalid hex"
else
    echo "❌ Test 5 FAILED: Server should return 400 for invalid hex"
fi

# Analyze Test 6: Size Mismatch
if grep -q "HTTP/1.1 400" results/chunked_bugs/test6_size_mismatch.txt; then
    echo "✅ Test 6 PASSED: Server detects chunk size mismatch"
else
    echo "❌ Test 6 FAILED: Server should detect chunk size mismatch"
fi

echo ""
echo "🔧 Bug Fix Recommendations:"
echo "========================="
echo "1. Fix validateChunkSizeLine() in HandleTransferChunks.cpp:"
echo "   - Parse chunk extensions properly (split on ';')"
echo "   - Only validate hex digits before semicolon"
echo ""
echo "2. Fix decodeChunk() end-detection logic:"
echo "   - Properly detect final chunk termination"
echo "   - Handle \\r\\n\\r\\n sequence correctly"
echo ""
echo "3. Add CRLF validation after chunk data:"
echo "   - Prevent infinite loops on malformed chunks"
echo "   - Return 400 Bad Request for missing CRLF"

echo ""
echo "📁 Test results saved in: results/chunked_bugs/"
