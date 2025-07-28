#!/usr/bin/env python3

import sys
import os
import urllib.parse
import html
import json

USER_DB_FILE = "/tmp/users.db"

def load_users():
    try:
        with open(USER_DB_FILE, "r") as f:
            return json.load(f)
    except Exception:
        return {}

def save_users(users):
    with open(USER_DB_FILE, "w") as f:
        json.dump(users, f)

def parse_post_data():
    length = int(os.environ.get("CONTENT_LENGTH", 0))
    raw_data = sys.stdin.read(length)
    return dict(urllib.parse.parse_qsl(raw_data))

def escape(s):
    return html.escape(s.strip())

def send_response(status=200, content="", username=None):
    status_line = {
        200: "Status: 200 OK",
        400: "Status: 400 Bad Request",
        401: "Status: 401 Unauthorized",
        405: "Status: 405 Method Not Allowed",
        409: "Status: 409 Conflict"
    }.get(status, f"Status: {status} Error")

    print(status_line)
    if username:
        # Use urllib.parse.quote_plus for cookie safe encoding
        safe_username = urllib.parse.quote_plus(username)
        print(f"Set-Cookie: username={safe_username}; Max-Age=3600; Path=/; HttpOnly")
    print("Content-Type: text/plain")
    print()
    print(content)

def main():
    method = os.environ.get("REQUEST_METHOD", "")
    if method != "POST":
        send_response(405, "Method Not Allowed")
        return

    form = parse_post_data()
    email = escape(form.get("email", ""))
    password = escape(form.get("password", ""))
    name = escape(form.get("name", ""))  # present only in signup

    if not email or not password:
        send_response(400, "Missing email or password.")
        return

    users = load_users()

    if name:
        # Signup flow
        if email in users:
            send_response(409, "User already exists.")
        else:
            users[email] = {"name": name, "password": password}
            save_users(users)
            send_response(200, f"Signup successful. Welcome, {name}!", name)
    else:
        # Login flow
        user = users.get(email)
        if user:
            if isinstance(user, dict):
                # New format
                if user.get("password") == password:
                    send_response(200, f"Welcome back, {user.get('name')}!", user.get("name"))
                    return
            else:
                # Old format: user is a password string
                if user == password:
                    username = email.split("@")[0]
                    send_response(200, f"Welcome back, {username}!", username)
                    return
        send_response(401, "Invalid credentials.")

if __name__ == "__main__":
    main()
