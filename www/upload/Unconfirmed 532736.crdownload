#!/usr/bin/env python3
import os

print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>Python CGI Info</h1>")
print("<p><b>Request Method:</b> " + os.environ.get("REQUEST_METHOD", "N/A") + "</p>")
print("<hr><h2>Environment Variables:</h2><pre>")
for key, value in os.environ.items():
    print(f"{key}: {value}")
print("</pre></body></html>")