#!/usr/bin/env python3
import os
import sys
import cgi
import cgitb
import shutil

cgitb.enable()

def main():
    # # === DEBUGGING SECTION ===
    # # Uncomment this block while testing to see environment info in the server's stderr log
    # content_type = os.environ.get('CONTENT_TYPE')
    # query_string = os.environ.get('QUERY_STRING')
    # upload_dir = os.environ.get('UPLOAD_STORE', './upload')
    # upload_url_base = os.environ.get('PUBLIC_URL_BASE', '/upload')
    
    # # print(f"DEBUG: CONTENT_TYPE = {content_type}", file=sys.stderr)
    # if content_type and 'boundary=' in content_type:
    #     boundary = content_type.split('boundary=')[1]
    # else:
    #     boundary = None
    # print(f"DEBUG: boundary = {boundary}", file=sys.stderr)
    # print(f"DEBUG: QUERY_STRING = {query_string}", file=sys.stderr)
    # print(f"DEBUG: UPLOAD_STORE = {upload_dir}", file=sys.stderr)
    # print(f"DEBUG: PUBLIC_URL_BASE = {upload_url_base}", file=sys.stderr)
    # print(f"DEBUG: contentlength = {os.environ.get('CONTENT_LENGTH')}", file=sys.stderr)

    # # === ACTUAL LOGIC ===
    upload_dir = os.environ.get('UPLOAD_STORE', './upload')
    public_url_base = os.environ.get('PUBLIC_URL_BASE', '/upload')

    if not os.path.exists(upload_dir):
        os.makedirs(upload_dir)

    form = cgi.FieldStorage()

    if "myfile" not in form:
        print("Status: 400 Bad Request")
        print("Content-Type: text/plain\n")
        print("No file uploaded.")
        return

    fileitem = form["myfile"]

    if not fileitem.file or not fileitem.filename:
        print("Status: 400 Bad Request")
        print("Content-Type: text/plain\n")
        print("No valid file uploaded.")
        return

    filename = os.path.basename(fileitem.filename)
    save_path = os.path.join(upload_dir, filename)

    # sleep(5)

    try:
        with open(save_path, "wb") as f:
            shutil.copyfileobj(fileitem.file, f)

        response_body = f".{public_url_base}/{filename}"
        content_length = len(response_body.encode('utf-8'))
        
        # Send proper HTTP headers with consistent \r\n line endings
        print("HTTP/1.1 200 OK\r")
        print("Content-Type: text/plain\r")
        print("Connection: keep-alive\r")
        print(f"Content-Length: {content_length}\r")
        print("\r")  # Empty line to end headers
        print(response_body, end="\n")

    except Exception as e:
        print("Status: 500 Internal Server Error")
        print("Content-Type: text/plain\n")
        print("Upload failed:", str(e))

if __name__ == "__main__":
    main()
