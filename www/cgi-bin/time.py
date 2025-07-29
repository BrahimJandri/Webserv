#!/usr/bin/env python3
import time
import sys

print("Content-Type: text/plain")
print()

# Simulate long processing time (longer than your server's timeout limit)
time.sleep(10)

print("This should never be seen if the timeout works.")
sys.exit(0)

