#!/bin/bash

{
    #<chunk-size>\r\n
    #<chunk-data>\r\n

    # HTTP headers
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: example.com\r\n"
    echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n\r\n"

    # Part 1: multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename="upload.txt"\r\nContent-Type: text/plain\r\n\r\nThis is the content of the file.'
    printf "%X\r\n" $(printf "%s" "$CHUNK1" | wc -c)    #size of chunk, HEX = 89, DEC = 137
    printf "%s\r\n" "$CHUNK1"                           #data of chunk
    sleep 1

    # Part 2: file content
    FILE_CONTENT=$'This is the content of the file.\nSecond line of text.\n'
    printf "%X\r\n" $(printf "%s" "$FILE_CONTENT" | wc -c)
    printf "%s\r\n" "$FILE_CONTENT"
    sleep 1

    # Part 3: ending boundary
    END_BOUNDARY=$'------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n'
    printf "%X\r\n" $(printf "%s" "$END_BOUNDARY" | wc -c)
    printf "%s\r\n" "$END_BOUNDARY"
    sleep 1

    # Final zero-length chunk
    echo -ne "0\r\n\r\n"

} | nc localhost 8080