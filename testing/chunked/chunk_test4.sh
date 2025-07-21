#!/bin/bash

{
    # HTTP headers
    echo -ne "POST /upload HTTP/1.1\r\n"
    echo -ne "Host: example.com\r\n"
    echo -ne "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    echo -ne "Transfer-Encoding: chunked\r\n\r\n"

    # Chunk 1: multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename="chunk_test4.txt"\r\nContent-Type: text/plain\r\n\r\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK1" | wc -c)
    printf "%s\r\n" "$CHUNK1"

    # Chunk 2: file content
    CHUNK2=$'First line of multi-chunk test.\nSecond line.\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK2" | wc -c)
    printf "%s\r\n" "$CHUNK2"

    # Chunk 3: more file content
    CHUNK3=$'Third line of multi-chunk test.\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK3" | wc -c)
    printf "%s\r\n" "$CHUNK3"

    # Ending boundary
    END_BOUNDARY=$'------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n'
    printf "%X\r\n" $(printf "%s" "$END_BOUNDARY" | wc -c)
    printf "%s\r\n" "$END_BOUNDARY"

    # Final zero-length chunk
    echo -ne "0\r\n\r\n"
} | nc localhost 8080