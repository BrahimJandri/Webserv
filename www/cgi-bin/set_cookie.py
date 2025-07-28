#!/usr/bin/env python3

import os
import sys
import json
from urllib.parse import quote
from datetime import datetime, timedelta

# Read JSON input from POST request
try:
    input_data = json.loads(sys.stdin.read())
except json.JSONDecodeError:
    print("Content-Type: application/json\r")
    print("\r")
    sys.stdout.write(json.dumps({
        "success": False,
        "error": "Invalid JSON input"
    }))
    sys.exit(1)

# Extract cookie parameters
cookie_name = input_data.get("name", "").strip()
cookie_value = input_data.get("value", "").strip()
expiry_seconds = input_data.get("expiry", None)

# Validate required fields
if not cookie_name or not cookie_value:
    print("Content-Type: application/json\r")
    print("\r")
    sys.stdout.write(json.dumps({
        "success": False,
        "error": "Cookie name and value are required"
    }))
    sys.exit(1)

# URL encode the cookie value to handle special characters
encoded_value = quote(cookie_value)

# Build cookie string
cookie_string = f"{cookie_name}={encoded_value}"

# Add expiry if specified
expiry_info = "Session"
if expiry_seconds:
    try:
        expiry_int = int(expiry_seconds)
        if expiry_int > 0:
            expiry_date = datetime.utcnow() + timedelta(seconds=expiry_int)
            cookie_string += f"; expires={expiry_date.strftime('%a, %d %b %Y %H:%M:%S GMT')}"
            expiry_info = f"{expiry_int} seconds"
    except (ValueError, TypeError):
        pass

# Add path for the cookie
cookie_string += "; path=/"

# Send HTTP headers with Set-Cookie
print("Content-Type: application/json\r")
print(f"Set-Cookie: {cookie_string}\r")
print("\r")

# Send JSON response
response = {
    "success": True,
    "message": "Cookie set successfully",
    "details": {
        "name": cookie_name,
        "value": cookie_value,
        "encoded_value": encoded_value,
        "expiry": expiry_info,
        "cookie_string": cookie_string
    }
}

sys.stdout.write(json.dumps(response, indent=2))