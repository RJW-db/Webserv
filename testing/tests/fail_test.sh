#!/bin/bash

cd /home/saleunin/Desktop/Webserv/testing 

# Clean and create directories for results
rm -rf results/post_fail
mkdir -p results/post_fail/uploads results/post_fail/small results/post_fail/tiny

echo "=== Starting POST Failure Tests ==="
echo "Make sure server is running with post_fail_tests.conf"
echo "These tests are designed to fail and test error handling"
echo ""

# Create test files for uploads
echo "Small test content" > /tmp/small_test.txt
dd if=/dev/zero of=/tmp/large_test.txt bs=1024 count=2 2>/dev/null  # 2KB file
dd if=/dev/zero of=/tmp/huge_test.txt bs=1024 count=10 2>/dev/null  # 10KB file

# Test 1: POST to non-existent server (connection should fail)
echo "1. Testing POST to non-existent server (should fail to connect)"
curl -i -X POST -H "Connection: close" -H "Host: nonexistent" \
    -F "file=@/tmp/small_test.txt" \
    http://localhost:99999/upload > results/post_fail/fail1.txt 2>&1 &
