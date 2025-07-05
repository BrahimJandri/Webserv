#!/usr/bin/env python3
import cgi, http.cookies, os

form = cgi.FieldStorage()
username = form.getvalue("username")
password = form.getvalue("password")

print("Content-Type: text/html")
if username == "admin" and password == "1234":
    print("Set-Cookie: sessionid=loggedin; Path=/")
    print()
    print('<html><body><h1>Login successful!</h1><a href="/session.html">Go to session page</a></body></html>')
else:
    print()
    print('<html><body><h1>Login failed!</h1><a href="/login.html">Try again</a></body></html>')
