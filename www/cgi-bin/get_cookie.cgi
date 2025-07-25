#!/usr/bin/env python3

import os
import sys
import json

from urllib.parse import unquote # decodeURIComponent



in_data = json.loads(sys.stdin.read())
if os.environ.get("HTTP_COOKIE") is None or os.environ.get("HTTP_COOKIE") == "":
    sys.stdout.write(json.dumps({
        "value": "no cookie header is provided !"
    }))
else:
    cookies = {}
    cookie = os.environ.get("HTTP_COOKIE")
    if ";" in cookie:
        for c in cookie.split(";"):
            pairs = c.split("=")
            cookies[pairs[0].strip()] = pairs[1].strip()
    else:
        pairs = cookie.split("=")
        cookies[pairs[0]] = pairs[1]
    try:
        val = cookies[in_data["name"]]
    except KeyError as e:
        val = "cookie not found!"
    sys.stdout.write(json.dumps({
        "value": unquote(val)
    }))