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
    # Part 1: partial multipart header
    CHUNK1=$'------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name="file"; filename='
    printf "%X\r\n" $(printf "%s" "$CHUNK1" | wc -c)
    printf "%s\r\n" "$CHUNK1"
    sleep 1

    # Part 2: rest of header and start of content
    CHUNK2=$'"chunk_test2.txt"\r\nContent-Type: text/plain\r\n\r\nThis is the content'
    printf "%X\r\n" $(printf "%s" "$CHUNK2" | wc -c)
    printf "%s\r\n" "$CHUNK2"
    sleep 1

    # Part 3: middle of file content
    CHUNK3=$' of the file.\nSecond line of'
    printf "%X\r\n" $(printf "%s" "$CHUNK3" | wc -c)
    printf "%s\r\n" "$CHUNK3"
    sleep 1

    # Part 4: more file content
    CHUNK4=$' text.\nthird line of text\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK4" | wc -c)
    printf "%s\r\n" "$CHUNK4"
    sleep 1

    # Part 5: start of ending boundary
    CHUNK5=$'------WebKitFormBoundary7MA4YWxkTrZu0gW-'
    printf "%X\r\n" $(printf "%s" "$CHUNK5" | wc -c)
    printf "%s\r\n" "$CHUNK5"
    sleep 1

    # Part 6: end of boundary
    CHUNK6=$'-\r\n'
    printf "%X\r\n" $(printf "%s" "$CHUNK6" | wc -c)
    printf "%s\r\n" "$CHUNK6"
    sleep 1

    # Final zero-length chunk
    echo -ne "0\r\n\r\n"

} | nc localhost 8080
