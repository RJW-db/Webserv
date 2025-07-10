cd /home/saleunin/Desktop/Webserv/testing 

mkdir -p results/post/upload1 results/post/upload2 results/post/upload3

# test 1: POST request to upload a file
echo "1. Testing POST request to upload test1.jpg to localhost:15000/upload"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: notexistinghost" -F "myfile=@expectedResults/post/upload1/test1.jpg" http://localhost:15000/upload1 > results/post/post1.txt &
echo -e "complete\n"

# Test 2: POST request to upload PNG file to server2
echo "2. Testing POST request to upload test2.png to localhost:15001/upload with Host header 'server2'"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server2" -F "myfile=@expectedResults/post/upload2/test2.png" http://localhost:15001/upload2  > results/post/post2.txt & 
echo -e "\n"

# Test 3: POST request to upload small text file to server3
echo "3. Testing POST request to upload small.txt to localhost:15000/upload with Host header 'server1'"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server1" -F "myfile=@expectedResults/post/upload1/small.txt" http://localhost:15000/upload1 > results/post/post3.txt &
echo -e "\n"

# Test 4: POST request to upload large text file to server1
echo "4. Testing POST request to upload 1M.txt to localhost:15000/upload with Host header 'server1'"
curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server1" -F "myfile=@expectedResults/post/upload3/1M.txt" http://localhost:15000/upload1 > results/post/post4.txt &
echo -e "\n"

# # Test 5: POST request to upload multiple files to server1
# echo "5. Testing POST request to upload multiple files (small2.txt and small3.txt) to localhost:15000/upload3 with Host header 'server1'"
# curl -i -X POST -H "Expect:" -H "Connection: close" -H "Host: server1" -F "file1=@expectedResults/post/upload1/small2.txt" -F "file2=@expectedResults/post/upload1/small3.txt" http://localhost:15000/upload3 > results/post/post5.txt &
# echo -e "\n"



sleep 3

# test 1: Check results
{
# test 1: Check results
if cmp -s expectedResults/post/post1.txt results/post/post1.txt && cmp -s expectedResults/post/upload1/test1.jpg results/post/upload1/test1.jpg; then
    echo "post test 1 completed successfully"
else
    cmp -s expectedResults/post/post1.txt results/post/post1.txt || echo "post test 1 failed because there is difference in expected for post.txt"
    cmp -s expectedResults/post/upload1/test1.jpg results/post/upload1/test1.jpg || echo "post test 1 failed because there is difference in expected for uploaded file"
fi

# test 2: Check results
if cmp -s expectedResults/post/post2.txt results/post/post2.txt && cmp -s expectedResults/post/upload2/test2.png results/post/upload2/test2.png; then
    echo "post test 2 completed successfully"
else
    cmp -s expectedResults/post/post2.txt results/post/post2.txt || echo "post test 2 failed because there is difference in expected for post.txt"
    cmp -s expectedResults/post/upload2/test2.png results/post/upload2/test2.png || echo "post test 2 failed because there is difference in expected for uploaded file"
fi

# test 3: Check results
if cmp -s expectedResults/post/post3.txt results/post/post3.txt && cmp -s expectedResults/post/upload1/small.txt results/post/upload1/small.txt; then
    echo "post test 3 completed successfully"
else
    cmp -s expectedResults/post/post3.txt results/post/post3.txt || echo "post test 3 failed because there is difference in expected for post.txt"
    cmp -s expectedResults/post/upload1/small.txt results/post/upload1/small.txt || echo "post test 3 failed because there is difference in expected for uploaded file"
fi

# test 4: Check results
if cmp -s expectedResults/post/post4.txt results/post/post4.txt && cmp -s expectedResults/post/upload3/1M.txt results/post/upload1/1M.txt; then
    echo "post test 4 completed successfully"
else
    cmp -s expectedResults/post/post4.txt results/post/post4.txt || echo "post test 4 failed because there is difference in expected for post.txt"
    cmp -s expectedResults/post/upload3/1M.txt results/post/upload1/1M.txt || echo "post test 4 failed because there is difference in expected for uploaded file"
fi

# # test 5: Check results for multiple file upload
# if cmp -s expectedResults/post/post5.txt results/post/post5.txt && cmp -s expectedResults/post/upload1/small2.txt results/post/upload3/small2.txt && cmp -s expectedResults/post/upload1/small3.txt results/post/upload3/small3.txt; then
#     echo "post test 5 completed successfully"
# else
#     cmp -s expectedResults/post/post5.txt results/post/post5.txt || echo "post test 5 failed because there is difference in expected for post.txt"
#     cmp -s expectedResults/post/upload1/small2.txt results/post/upload3/small2.txt || echo "post test 5 failed because there is difference in expected for small2.txt"
#     cmp -s expectedResults/post/upload1/small3.txt results/post/upload3/small3.txt || echo "post test 5 failed because there is difference in expected for small3.txt"
# fi

} > results/post/summary.txt 2>&1

echo "Post tests completed"
exit 0