#!/usr/bin/env python3

import base64
import json
import sys

from typing import Callable



bases: dict[str | Callable] = {
    "base85": base64.b85encode,
    "base16": base64.b16encode,
    "base32": base64.b32encode,
    "base64": base64.b64encode,
}
in_data = json.loads(sys.stdin.read())

try:
    encoded_cipher_s = bases[in_data["alg"]](str(in_data["cipher"]).encode())
except KeyError as _:
    encoded_cipher_s = b"error!"

json_response = {
    "encoded": encoded_cipher_s.decode()
}
sys.stdout.write(json.dumps(json_response))