#!/usr/bin/env python3
import os
import sys

# HTTP header is crucial for CGI output
print("Content-Type: text/html\r\n")
print("<!DOCTYPE html><html><head><title>CGI Output</title>")
print("<style>body{font-family: sans-serif; background-color: #e0f7fa; padding: 20px;} pre{background-color: #fff; padding: 10px; border: 1px solid #ccc;}</style>")
print("</head><body><h1>Hello from CGI!</h1>")
print("<p>This is a Python CGI script executed by your webserv.</p>")

print("<h2>Environment Variables:</h2><pre>")
for key, value in os.environ.items():
    print(f"{key}: {value}")
print("</pre>")

if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        print(f"<h2>POST Data Received:</h2><pre>{post_data}</pre>")

print("</body></html>")