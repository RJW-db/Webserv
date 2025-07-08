# cd /home/saleunin/Desktop/Webserv/testing 

mkdir -p results/post/upload1 results/post/upload2 results/post/upload3

# # test 1: POST request to upload a file
# echo "1. Testing POST request to upload test1.jpg to localhost:15000/upload"
# curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: notexistinghost" -F "myfile=@expectedResults/post/upload1/test1.jpg" http://localhost:15000/upload1 > results/post/post1.txt &
# echo -e "\n"

# # Test 2: POST request to upload PNG file to server2
# echo "2. Testing POST request to upload test2.png to localhost:15001/upload with Host header 'server2'"
# curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server2" -F "myfile=@expectedResults/post/upload2/test2.png" http://localhost:15001/upload2  > results/post/post2.txt & 
# echo -e "\n"

# # Test 3: POST request to upload small text file to server3
# echo "3. Testing POST request to upload small.txt to localhost:15000/upload with Host header 'server1'"
# curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server1" -F "myfile=@expectedResults/post/upload1/small.txt" http://localhost:15000/upload1 > results/post/post3.txt &
# echo -e "\n"

# # Test 4: POST request to upload large text file to server1
# echo "4. Testing POST request to upload 1M.txt to localhost:15000/upload with Host header 'server1'"
# curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server1" -F "myfile=@expectedResults/post/upload3/1M.txt" http://localhost:15000/upload1 > results/post/post4.txt &
# echo -e "\n"


# # error posts
# # Test 5: POST request to upload large text file to server3
# echo "5. Testing POST request to upload 1M.txt to localhost:15000/upload with Host header 'server3' which should fail due to size limit"
# curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server3" -F "myfile=@expectedResults/post/upload3/1M.txt" http://localhost:15000/upload3 > results/post/post5.txt &
# echo -e "\n"

# Create a small test file for chunked upload
# echo -e "This is line 1\nThis is line 2\nThis is line 3" > chunked_test.txt


# POST /upload HTTP/1.1\r\n
# Host: example.com\r\n
# Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n
# Transfer-Encoding: chunked\r\n
# \r\n
# 4A\r\n
# ------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n
# Content-Disposition: form-data; name="file"; filename="hello.txt"\r\n
# Content-Type: text/plain\r\n
# \r\n
# 14\r\n
# Hello, Webserv!\r\n
# 28\r\n
# ------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n
# \r\n
# 0\r\n
# \r\n


# Test 6: POST chunked request with a file as body
# echo "6. Testing POST chunked request with multiple small chunks to localhost:15000/upload1"
# (
#     echo -n "chunk1"
#     sleep 1
#     echo -n "chunk2"
#     sleep 1
#     echo -n "chunk3"
# ) | curl -i -X POST \
#     -H "Transfer-Encoding: chunked" \
#     -H "Content-Type: text/plain" \
#     --data-binary @- \
#     http://localhost:15000/upload1 > results/post/post6.txt &
# echo -e "\n"

sleep 3

# # test 1: Check results
# {
# # test 1: Check results
# if cmp -s expectedResults/post/post1.txt results/post/post1.txt && cmp -s expectedResults/post/upload1/test1.jpg results/post/upload1/test1.jpg; then
#     echo "post test 1 completed successfully"
# else
#     cmp -s expectedResults/post/post1.txt results/post/post1.txt || echo "post test 1 failed because there is difference in expected for post.txt"
#     cmp -s expectedResults/post/upload1/test1.jpg results/post/upload1/test1.jpg || echo "post test 1 failed because there is difference in expected for uploaded file"
# fi

# # test 2: Check results
# if cmp -s expectedResults/post/post2.txt results/post/post2.txt && cmp -s expectedResults/post/upload2/test2.png results/post/upload2/test2.png; then
#     echo "post test 2 completed successfully"
# else
#     cmp -s expectedResults/post/post2.txt results/post/post2.txt || echo "post test 2 failed because there is difference in expected for post.txt"
#     cmp -s expectedResults/post/upload2/test2.png results/post/upload2/test2.png || echo "post test 2 failed because there is difference in expected for uploaded file"
# fi

# # test 3: Check results
# if cmp -s expectedResults/post/post3.txt results/post/post3.txt && cmp -s expectedResults/post/upload1/small.txt results/post/upload1/small.txt; then
#     echo "post test 3 completed successfully"
# else
#     cmp -s expectedResults/post/post3.txt results/post/post3.txt || echo "post test 3 failed because there is difference in expected for post.txt"
#     cmp -s expectedResults/post/upload1/small.txt results/post/upload1/small.txt || echo "post test 3 failed because there is difference in expected for uploaded file"
# fi

# # test 4: Check results
# if cmp -s expectedResults/post/post4.txt results/post/post4.txt && cmp -s expectedResults/post/upload3/1M.txt results/post/upload1/1M.txt; then
#     echo "post test 4 completed successfully"
# else
#     cmp -s expectedResults/post/post4.txt results/post/post4.txt || echo "post test 4 failed because there is difference in expected for post.txt"
#     cmp -s expectedResults/post/upload3/1M.txt results/post/upload1/1M.txt || echo "post test 4 failed because there is difference in expected for uploaded file"
# fi

# # test 5: Check results
# if cmp -s expectedResults/post/post5.txt results/post/post5.txt; then
#     echo "post test 5 completed successfully"
# else
#     cmp -s expectedResults/post/post5.txt results/post/post5.txt || echo "post test 5 failed because there is difference in expected for post.txt"
# fi
# } > results/post/summary.txt 2>&1

echo "Post tests completed"