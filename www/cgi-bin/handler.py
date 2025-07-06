#!/usr/bin/env python3
import sys
import cgi

print("Content-Type: text/html\r\n\r\n")
print("<html><body><h1>Python POST Handler</h1>")

form = cgi.FieldStorage()
if "name" in form:
    name = form.getvalue("name")
    print(f"<p>Hello, <strong>{name}</strong>!</p>")
else:
    print("<p>No name was submitted.</p>")
    
print("<a href='/'>Go back</a>")
print("</body></html>")