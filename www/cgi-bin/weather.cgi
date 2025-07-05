#!/usr/bin/env python3
import cgi, os, json, http.client
from http import cookies
data=json.loads(cgi.FieldStorage().file.read().decode())
city=data['city']
# Ideally call real weather API, but here's static stub:
resp={'city':city,'temperature':25,'humidity':50,'wind':10}
print("Content-Type: application/json\n")
print(json.dumps(resp))
