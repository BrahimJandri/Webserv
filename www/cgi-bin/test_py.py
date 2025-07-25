#!/usr/bin/env python3

import os
import sys
import urllib.parse # For URL decoding

# Define the path to your error page
ERROR_PAGE_PATH = "/error.html"

request_method = os.environ.get('REQUEST_METHOD')
content_type = os.environ.get('CONTENT_TYPE', '')

# Read the POST body first, as it's needed regardless of redirect logic
post_body = ""
if request_method == 'POST':
    post_body = sys.stdin.read()

# --- REDIRECTION LOGIC ---
# Check for a specific condition that should trigger a redirect
# For this example, if 'py_message' contains "redirect", we'll redirect.
should_redirect = False
if request_method == 'POST' and 'application/x-www-form-urlencoded' in content_type:
    if post_body:
        parsed_data = urllib.parse.parse_qs(post_body)
        py_message = parsed_data.get('py_message', [''])[0] # Get the first value
        if "redirect" in py_message.lower():
            should_redirect = True

if should_redirect:
    # Output HTTP headers for redirection
    # Status: 302 Found is a common temporary redirect status
    print("Status: 302 Found")
    print(f"Location: {ERROR_PAGE_PATH}")
    print("") # Empty line to end headers
    sys.exit(0) # Exit immediately after sending redirect headers
# --- END REDIRECTION LOGIC ---

# If no redirect, proceed with normal CGI output
print("Content-Type: text/plain")
print("") # Empty line to end headers

print("Hello from Python CGI (Updated)!")
print(f"Request Method: {os.environ.get('REQUEST_METHOD')}")
print(f"Script Name: {os.environ.get('SCRIPT_NAME')}")
print(f"Query String: {os.environ.get('QUERY_STRING')}")
print(f"Content-Length: {os.environ.get('CONTENT_LENGTH')}")
print(f"Content-Type: {os.environ.get('CONTENT_TYPE')}")
print(f"HTTP_USER_AGENT: {os.environ.get('HTTP_USER_AGENT')}")


print("\n--- Received POST Data ---")
if post_body:
    if 'application/x-www-form-urlencoded' in content_type:
        parsed_data = urllib.parse.parse_qs(post_body)
        print("Parsed Form Data:")
        for key, values in parsed_data.items():
            print(f"  '{key}': '{values[0]}'") # Assuming single value for simplicity
    else:
        print(f"Raw POST body (Content-Type: {content_type}):")
        print(post_body)
else:
    print("(No POST data received or body was empty)")
print("--------------------------")

