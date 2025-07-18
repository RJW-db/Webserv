#!/bin/bash

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
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename="upload.txt"\r\nContent-Type: text/plain\r\n\r\n'
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
    # echo -ne "0\r\n\r\n"

} | nc localhost 8080



# {
#     echo -ne "POST /upload HTTP/1.1\r\nHost: example.com\r\nContent-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\nTransfer-Encoding: chunked\r\n\r\n"
#     echo "Sent header" >&2
#     sleep 1
#     echo -ne "4A\r\n------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
#     echo "Sent 1st chunk" >&2
#     sleep 1
#     echo -ne "13\r\nHello, Webserv\r\n"
#     echo "Sent 2nd chunk" >&2
#     sleep 1
#     echo -ne "29\r\n------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n"
#     echo "Sent 3rd chunk" >&2
#     sleep 1
#     echo -ne "0\r\n\r\n"
#     echo "Sent last chunk" >&2
# } | nc localhost 8080

# echo -ne
# -n for no new line
# -e to print \r & \n as a whitepace


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
# 13\r\n
# Hello, Webserv\r\n
# 29\r\n
# ------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n
# 0\r\n
# \r\n