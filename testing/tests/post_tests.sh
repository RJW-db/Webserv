cd /home/saleunin/Desktop/Webserv/testing 

mkdir -p results/post/upload1 results/post/upload2 results/post/upload3

# test 1: POST request to upload a file
echo "1. Testing POST request to upload test1.jpg to localhost:15000/upload"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: notexistinghost" -F "myfile=@expectedResults/post/upload1/test1.jpg" http://localhost:15000/upload1 > results/post/post1.txt &

# Test 2: POST request to upload PNG file to server2
echo "2. Testing POST request to upload test2.png to localhost:15000/upload with Host header 'server2'"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server2" -F "myfile=@expectedResults/post/upload1/test2.png" http://localhost:15000/upload1  > results/post/post2.txt & 

# Test 3: POST request to upload small text file to server3
echo "3. Testing POST request to upload small.txt to localhost:15000/upload with Host header 'server1'"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server1" -F "myfile=@expectedResults/post/upload1/small.txt" http://localhost:15000/upload1 > results/post/post3.txt &

# Test 4: POST request to upload large text file to server1
echo "4. Testing POST request to upload 1M.txt to localhost:15000/upload with Host header 'server1'"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server1" -F "myfile=@expectedResults/post/upload1/1M.txt" http://localhost:15000/upload1 > results/post/post4.txt &

# Test 5: POST request to upload multiple files to server1
echo "5. Testing POST request to upload multiple files (small2.txt and small3.txt) to localhost:15001/upload3 with Host header 'server1'"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server2" \
-F "file1=@expectedResults/post/upload1/small.txt" -F "file2=@expectedResults/post/upload1/small2.txt" \
-F "file3=@expectedResults/post/upload1/small3.txt" -F "file4=@expectedResults/post/upload1/test1.jpg" \
-F "file5=@expectedResults/post/upload1/test2.png" \
 http://localhost:15001/upload2 > results/post/post5.txt &

# test 6 : chunked post request to upload small.txt to server1
echo "6. Testing chunked POST request to upload small.txt to localhost:15000/upload with Host header 'server1'"
curl -i -X POST -H "Transfer-Encoding: chunked" -H "Expect:" -H "Connection: close" -H "Host: server1" \
--data-binary @expectedResults/post/upload1/small.txt http://localhost:15000/upload3 > results/post/post6.txt &

sleep 2

{

# test 1: Check results
# Extract the filename from the server response (last line of the response)
actual_filename1=$(tail -n 1 results/post/post1.txt | tr -d '\r\n')
expected_response_content="./testing/results/post/upload1/test1.jpg"

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" results/post/post1.txt && grep -q "Content-Type: text/plain" results/post/post1.txt; then
    if [[ -f "$actual_filename1" ]] && cmp -s expectedResults/post/upload1/test1.jpg "$actual_filename1"; then
        echo "post test 1 completed successfully"
    else
        echo "post test 1 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename1" ]] && echo "  - File $actual_filename1 does not exist"
    fi
else
    echo "post test 1 failed because HTTP response headers are incorrect"
fi

# test 2: Check results
# Extract the filename from the server response (last line of the response)
actual_filename2=$(tail -n 1 results/post/post2.txt | tr -d '\r\n')

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" results/post/post2.txt && grep -q "Content-Type: text/plain" results/post/post2.txt; then
    if [[ -f "$actual_filename2" ]] && cmp -s expectedResults/post/upload1/test2.png "$actual_filename2"; then
        echo "post test 2 completed successfully"
    else
        echo "post test 2 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename2" ]] && echo "  - File $actual_filename2 does not exist"
    fi
else
    echo "post test 2 failed because HTTP response headers are incorrect"
fi

# test 3: Check results
# Extract the filename from the server response (last line of the response)
actual_filename3=$(tail -n 1 results/post/post3.txt | tr -d '\r\n')

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" results/post/post3.txt && grep -q "Content-Type: text/plain" results/post/post3.txt; then
    if [[ -f "$actual_filename3" ]] && cmp -s expectedResults/post/upload1/small.txt "$actual_filename3"; then
        echo "post test 3 completed successfully"
    else
        echo "post test 3 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename3" ]] && echo "  - File $actual_filename3 does not exist"
    fi
else
    echo "post test 3 failed because HTTP response headers are incorrect"
fi

# test 4: Check results
# Extract the filename from the server response (last line of the response)
actual_filename4=$(tail -n 1 results/post/post4.txt | tr -d '\r\n')

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" results/post/post4.txt && grep -q "Content-Type: text/plain" results/post/post4.txt; then
    if [[ -f "$actual_filename4" ]] && cmp -s expectedResults/post/upload1/1M.txt "$actual_filename4"; then
        echo "post test 4 completed successfully"
    else
        echo "post test 4 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename4" ]] && echo "  - File $actual_filename4 does not exist"
    fi
else
    echo "post test 4 failed because HTTP response headers are incorrect"
fi

# test 5: Check results for multiple file upload
all_passed=true

# Check if response headers are correct
if ! grep -q "HTTP/1.1 201 Created" results/post/post5.txt || ! grep -q "Content-Type: text/plain" results/post/post5.txt; then
    echo "post test 5 failed because HTTP response headers are incorrect"
    all_passed=false
fi

# For multiple file uploads, we need to find the actual uploaded files
# The server response contains the path of the last uploaded file
last_filename=$(tail -n 1 results/post/post5.txt | tr -d '\r\n')

# Extract the directory from the last filename
if [[ -n "$last_filename" ]]; then
    upload_dir=$(dirname "$last_filename")
    
    # Check if all files were uploaded by looking for files in the upload directory
    expected_files=("small.txt" "small2.txt" "small3.txt" "test1.jpg" "test2.png")
    
    for expected_file in "${expected_files[@]}"; do
        found_match=false
        # Look for files in the upload directory that match the content
        for actual_file in "$upload_dir"/*; do
            if [[ -f "$actual_file" ]] && cmp -s "expectedResults/post/upload1/$expected_file" "$actual_file"; then
                found_match=true
                break
            fi
        done
        
        if ! $found_match; then
            echo "post test 5 failed because $expected_file was not found or content doesn't match"
            all_passed=false
        fi
    done
else
    echo "post test 5 failed because no filename was returned in response"
    all_passed=false
fi

if $all_passed; then
    echo "post test 5 completed successfully"
fi

# test 6: Check results for chunked transfer
# Extract the filename from the server response (last line of the response)
actual_filename6=$(tail -n 1 results/post/post6.txt | tr -d '\r\n')

# Check if response headers are correct and if the file content matches
if grep -q "HTTP/1.1 201 Created" results/post/post6.txt && grep -q "Content-Type: text/plain" results/post/post6.txt; then
    if [[ -f "$actual_filename6" ]] && cmp -s expectedResults/post/upload1/small.txt "$actual_filename6"; then
        echo "post test 6 completed successfully"
    else
        echo "post test 6 failed because uploaded file content doesn't match expected content"
        [[ ! -f "$actual_filename6" ]] && echo "  - File $actual_filename6 does not exist"
    fi
else
    echo "post test 6 failed because HTTP response headers are incorrect"
fi

} > results/post/summary.txt 2>&1

echo "Post tests completed"
exit 0