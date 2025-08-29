#!/usr/bin/env python3
import os
import sys
import cgi
import cgitb
import shutil
import time
import signal

# Ignore SIGPIPE so Python doesn't print BrokenPipeError to stderr
signal.signal(signal.SIGPIPE, signal.SIG_DFL)

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


    # time.sleep(15)

    if os.environ.get('REQUEST_METHOD', '') == 'POST':
        upload_dir = os.environ.get('UPLOAD_STORE', './upload')
        public_url_base = os.environ.get('PUBLIC_URL_BASE', '/upload')

        if not os.path.exists(upload_dir):
            os.makedirs(upload_dir)

        form = cgi.FieldStorage()

        if "myfile" not in form:
            print("Status: 400 Bad Request\r")
            print("Content-Type: text/plain\r\n\r")
            print("No file uploaded.")
            return

        fileitem = form["myfile"]

        if not fileitem.file or not fileitem.filename:
            print("Status: 400 Bad Request\r")
            print("Content-Type: text/plain\r\n\r")
            print("No valid file uploaded.")
            return

        filename = os.path.basename(fileitem.filename)
        save_path = os.path.join(upload_dir, filename)

        # time.sleep(15)

        try:
            with open(save_path, "wb") as f:
                shutil.copyfileobj(fileitem.file, f)

            response_body = f".{public_url_base}/{filename}"
            content_length = len(response_body.encode('utf-8'))
            
            # Send proper HTTP headers with consistent \r\n line endings
            print("Status: 200 OK\r")
            print("Content-Type: text/plain\r")
            print("Connection: keep-alive\r")
            print(f"Content-Length: {content_length}\r")
            print("\r")  # Empty line to end headers
            print(response_body, end="\n")

        except Exception as e:
            print("Status: 500 Internal Server Error\r")
            print("Content-Type: text/plain\r\n\r")
            print("Upload failed:", str(e))
    elif os.environ.get('REQUEST_METHOD', '') == 'GET':
        print("Status: 405 Method Not Allowed\r")
        print("Content-Type: text/plain\r\n\r")
        print("GET method is not allowed for this endpoint.")
    elif os.environ.get('REQUEST_METHOD', '') == 'DELETE':
        print(f"DEBUG: this works", file=sys.stderr)
        
        filename = os.environ.get('QUERY_STRING', '').replace('filename=', '')
        upload_dir = os.environ.get('UPLOAD_STORE', './upload')
        file_path = os.path.join(upload_dir, os.path.basename(filename))

        if not filename:
            print("Status: 400 Bad Request\r")
            print("Content-Type: text/plain\r\n\r")
            print("No filename provided.")
            return
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
                print("Status: 200 OK\r")
                print("Content-Type: text/plain\r\n\r")
                print("File deleted successfully.")
            except Exception as e:
                print("Status: 500 Internal Server Error\r")
                print("Content-Type: text/plain\r\n\r")
                print(f"Failed to delete file: {e}")
        else:
            print("Status: 404 Not Found\r")
            print("Content-Type: text/plain\r\n\r")
            print("File not found.")
        print(f"DEBUG: end of python", file=sys.stderr)
    else:
        print("Status: 501 Not Implemented\r")
        print("Content-Type: text/plain\r\n\r")
        print("This method is not implemented for this endpoint.")

if __name__ == "__main__":
    main()
    sys.exit(0)
